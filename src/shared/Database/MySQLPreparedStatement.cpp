/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Database/DatabaseEnv.h"
#include "Database/SqlOperations.h"

#ifndef DO_POSTGRESQL

#define MAX_NR_ARGUMENTS 2048

MySQLPreparedStatement::MySQLPreparedStatement(DatabaseMysql *db, const char *sql, va_list ap)
{
    ZThread::Guard<ZThread::FastMutex> query_connection_guard(db->mMutex);

    m_db = db;
    m_stmt = mysql_stmt_init(m_db->mMysql);
    if(!m_stmt)
    {
        sLog.outError("mysql_stmt_init(), out of memory");
        assert(false);
    }

    size_t sql_len = strlen(sql);
    char *stmt_str = new char[sql_len+1], *p, *q;

    format = (enum_field_types*)malloc(sql_len*sizeof(enum_field_types));
    format_len = 0;
    nr_strings = 0;
    
    for(p = (char*)sql, q = stmt_str; *p != '\0'; p++, q++)
    {
        if(*p == '%')
        {
            p++;
            if(*p != '%')
            {
                if(*p == '\0')
                    break;

                bool length_mod = false;
                bool repeat;

                *q = '?';
                do
                {
                    repeat = false;

                    switch(*p)
                    {
                        case 'b':
                            nr_strings++;
                            format[format_len++] = MYSQL_TYPE_BLOB;
                            break;
                        case 'c':
                            format[format_len++] = MYSQL_TYPE_TINY;
                            break;
                        case 'd':
                        case 'i':
                        case 'u':
                            format[format_len++] = MYSQL_TYPE_LONG;
                            break;
                        case 'f':
                            format[format_len++] = length_mod ? MYSQL_TYPE_DOUBLE : MYSQL_TYPE_FLOAT;
                            break;
                        case 's':
                            nr_strings++;
                            format[format_len++] = MYSQL_TYPE_STRING;
                            break;
                        case 'l':
                            if(!length_mod)
                            {
                                length_mod = true;
                                repeat = true;
                                p++;
                                break;
                            }
                            // error otherwise
                        default:
                            sLog.outError("MySQLPreparedStatement: unsupported format specified %c", *p);
                            assert(false);
                            break;
                    }
                }
                while(repeat == true);
            }
            else
                *q = '%';
        }
        else
            *q = *p;
    }
    *q = '\0';

    assert(format_len <= MAX_NR_ARGUMENTS);

    if(format_len == 0)
    {
        free(format);
        format = NULL;
    }
    else
        format = (enum_field_types*)realloc(format, format_len*sizeof(enum_field_types));

    if(mysql_stmt_prepare(m_stmt, stmt_str, q - stmt_str))
    {
        sLog.outError("mysql_stmt_prepare(), %s", mysql_stmt_error(m_stmt));
        assert(false);
    }
    delete[] stmt_str;

    m_bind = new MYSQL_BIND[format_len];
    m_data = new uint64[format_len + nr_strings];
    m_str_idx = new int[format_len];
    m_bufs = new char*[nr_strings];

    memset(m_bind, 0, format_len*sizeof(MYSQL_BIND));

    int poz = 0;
    int str_poz = 0;
    uint32 buf_len;

    for(int i = 0; i < format_len; i++, poz++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_STRING:
                buf_len = va_arg(ap, uint32);
                m_bufs[str_poz] = new char[buf_len];
                _set_bind(m_bind[i], format[i], m_bufs[str_poz], buf_len, (unsigned long*)&m_data[poz]);
                m_str_idx[str_poz++] = i;
                break;
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_LONGLONG:
                _set_bind(m_bind[i], format[i], (char*)&m_data[poz], 0, NULL);
                break;
        }
    }

    if(mysql_stmt_bind_param(m_stmt, m_bind))
    {
        sLog.outError("mysql_stmt_bind_param() failed, %s", mysql_stmt_error(m_stmt));
        assert(false);
    }
}

void MySQLPreparedStatement::_set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len)
{
    bind.buffer_type = type;
    bind.buffer = value;
    bind.buffer_length = buf_len;
    bind.length = len;
}

MySQLPreparedStatement::~MySQLPreparedStatement()
{
    // called from Database::~Database
    free(format);
    if(mysql_stmt_close(m_stmt))
        sLog.outError("failed while closing the prepared statement");
    delete[] m_bind;
    delete[] m_data;
    delete[] m_str_idx;
    for(int i = 0; i < nr_strings; i++)
        delete[] m_bufs[i];
    delete[] m_bufs;
}

bool MySQLPreparedStatement::DirectExecute()
{
    ZThread::Guard<ZThread::FastMutex> query_connection_guard(m_db->mMutex);

    if(mysql_stmt_execute(m_stmt))
    {
        // can this occur on query syntax error ?
        sLog.outError("mysql_stmt_execute() failed, %s", mysql_stmt_error(m_stmt));
        assert(false); // test
        return false;
    }

    return true;
}

bool MySQLPreparedStatement::Execute()
{
    return Execute(NULL);
}

bool MySQLPreparedStatement::DirectExecute(char *raw_data)
{
    memcpy(m_data, raw_data, format_len * sizeof(uint64));
    char **bufs = (char**)&raw_data[format_len * sizeof(uint64)];

    for(int i = 0; i < nr_strings; i++)
        memcpy(m_bufs[i], bufs[i], *(uint32*)&m_data[m_str_idx[i]]);
    
    return DirectExecute();
}

bool MySQLPreparedStatement::Execute(char *raw_data)
{
    // TODO: move this into database and reuse this code
    if (!m_db->mMysql)
        return false;

    // don't use queued execution if it has not been initialized
    if (!m_db->m_threadBody) return DirectExecute(raw_data);

    m_db->tranThread = ZThread::ThreadImpl::current();      // owner of this transaction
    TransactionQueues::iterator i = m_db->m_tranQueues.find(m_db->tranThread);
    if (i != m_db->m_tranQueues.end() && i->second != NULL)
    {                                                       // Statement for transaction
        i->second->DelayExecute(this, raw_data);
    }
    else
    {
        m_db->m_threadBody->Delay(new SqlPreparedStatement(this, raw_data));
    }
    
    return true;
}

void MySQLPreparedStatement::Free(char *raw_data)
{
    char **bufs = (char**)&raw_data[format_len*sizeof(uint64)];
    // all strings are stored in one buffer
    // the first string points to the start of the buffer
    if(nr_strings > 0)
        delete[] bufs[0];
    delete[] raw_data;
}

template< class B >
void MySQLPreparedStatement::_parse_args(B &binder, void *arg1, va_list ap)
{
    uint32 buf_len;
    switch(format[0])
    {
        case MYSQL_TYPE_BLOB:
            binder.append(*(uint32*)arg1, va_arg(ap, char*));
            break;
        case MYSQL_TYPE_TINY:
            binder << *(char*)arg1;
            break;
        case MYSQL_TYPE_LONG:
            binder << *(unsigned long*)arg1;
            break;
        case MYSQL_TYPE_FLOAT:
            binder << *(float*)arg1;
            break;
        case MYSQL_TYPE_DOUBLE:
            binder << *(double*)arg1;
            break;
        case MYSQL_TYPE_STRING:
            binder << *(char**)arg1;
            break;
    }

    for(int i = 1; i < format_len; i++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
                // argument evaluation order is unspecified, do this first
                buf_len = va_arg(ap, uint32);
                binder.append(buf_len, va_arg(ap, char*));
                break;
            case MYSQL_TYPE_TINY:
                binder << va_arg(ap, char);
                break;
            case MYSQL_TYPE_LONG:
                binder << va_arg(ap, unsigned long);
                break;
            case MYSQL_TYPE_FLOAT:
                binder << va_arg(ap, float);
                break;
            case MYSQL_TYPE_DOUBLE:
                binder << va_arg(ap, double);
                break;
            case MYSQL_TYPE_STRING:
                binder << va_arg(ap, char*);
                break;
        }
    }
}

bool MySQLPreparedStatement::_DirectPExecute(void *arg1, va_list ap)
{
    MySQLPreparedStatementDirectBinder binder(this);
    _parse_args(binder, arg1, ap);
    return binder.DirectExecute();
}

bool MySQLPreparedStatement::_PExecute(void *arg1, va_list ap)
{
    MySQLPreparedStatementBinder binder(this);
    _parse_args(binder, arg1, ap);
    return binder.Execute();
}

QueryResult * MySQLPreparedStatement::Query()
{
    return NULL;
}

QueryResult * MySQLPreparedStatement::_PQuery(void *arg1, va_list ap)
{
    return NULL;
}

MySQLPreparedStatementBinder::MySQLPreparedStatementBinder(MySQLPreparedStatement *stmt)
    : m_stmt(stmt), m_poz(0), m_str_poz(0), m_total_buf_len(0)
{
    m_data = (uint64*)new char[m_stmt->format_len*sizeof(uint64)+m_stmt->nr_strings*sizeof(char*)];
    m_bufs = (char**)&m_data[m_stmt->format_len];
}

bool MySQLPreparedStatementBinder::Execute()
{
    assert(m_poz == m_stmt->format_len && m_str_poz == m_stmt->nr_strings);
    
    if(m_stmt->nr_strings > 0)
    {
        // all strings are stored in one buffer
        // the first string points to the start of the buffer
        char *buf = new char[m_total_buf_len];

        int j = 0;
        for(int i = 0; i < m_stmt->nr_strings; i++)
        {
            uint32 format_idx = m_stmt->m_str_idx[i];
            uint32 len = *(uint32*)&m_data[format_idx];
            // one extra byte for the terminating '\0'
            if(m_stmt->format[format_idx] == MYSQL_TYPE_STRING)
                len++;

            memcpy(&buf[j], m_bufs[i], len);
            m_bufs[i] = &buf[j];
            j += len;
        }
    }

    return m_stmt->Execute((char*)m_data);
}

MySQLPreparedStatementDirectBinder::MySQLPreparedStatementDirectBinder(MySQLPreparedStatement *stmt)
    : m_stmt(stmt), m_poz(0), m_str_poz(0)
{
}

bool MySQLPreparedStatementDirectBinder::DirectExecute()
{
    return m_stmt->DirectExecute();
}

#endif
