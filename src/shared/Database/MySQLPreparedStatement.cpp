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

                *q = '?';
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
                        format[format_len++] = MYSQL_TYPE_FLOAT;
                        break;
                    case 's':
                        nr_strings++;
                        format[format_len++] = MYSQL_TYPE_STRING;
                        break;
                    default:
                        sLog.outError("MySQLPreparedStatement: unsupported format specified %c", *p);
                        assert(false);
                        break;
                }
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

    int poz = 0;
    int str_poz = 0;

    for(int i = 0; i < format_len; i++, poz++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_STRING:
                _set_bind(m_bind[i], format[i], NULL, va_arg(ap, uint32), (unsigned long*)&m_data[poz]);
                m_str_idx[str_poz++] = i;
                break;
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_FLOAT:
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

MySQLPreparedStatement::~MySQLPreparedStatement()
{
    free(format);
    if(mysql_stmt_close(m_stmt))
        sLog.outError("failed while closing the prepared statement");
}

void MySQLPreparedStatement::DirectExecute()
{
    if(mysql_stmt_execute(m_stmt))
    {
        // can this occur on query syntax error ?
        sLog.outError("mysql_stmt_execute() failed, %s", mysql_stmt_error(m_stmt));
        assert(false);
    }
}

void MySQLPreparedStatement::Execute()
{
    Execute(NULL);
}

void MySQLPreparedStatement::Execute(char *raw_data)
{
    // TODO: transactions!
    m_db->m_threadBody->Delay(new SqlPreparedStatement(this, raw_data));
}

void MySQLPreparedStatement::DirectExecute(char *raw_data)
{
    memcpy(m_data, raw_data, format_len * sizeof(uint64));
    char **bufs = (char**)&m_data[format_len];

    for(int i = 0; i < nr_strings; i++)
        m_bind[m_str_idx[i]].buffer = bufs[i];
    
    DirectExecute();
}

QueryResult * MySQLPreparedStatement::Query()
{
    return NULL;
}

void MySQLPreparedStatement::_set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len)
{
    bind.buffer_type = type;
    bind.buffer = value;
    bind.buffer_length = buf_len;
    bind.length = len;
}

void MySQLPreparedStatement::_PExecute(void *arg1, va_list ap)
{
    char * raw_data = new char[format_len*sizeof(uint64)+nr_strings*sizeof(char*)];
    uint64 *data = (uint64*)raw_data;
    char **bufs = (char**)&data[format_len];

    uint32 buf_len[MAX_NR_ARGUMENTS];
    char *aux;

    int total_buf_len = 0;
    int i, j = 0;
    switch(format[0])
    {
        case MYSQL_TYPE_BLOB:
            buf_len[j] = *(uint32*)arg1;
            bufs[j] = va_arg(ap, char*);
            *(uint32*)&data[0] = buf_len[j];
            total_buf_len += buf_len[j];
            j++;
            break;
        case MYSQL_TYPE_TINY:
            *(char*)&data[0] = *(char*)arg1;
            break;
        case MYSQL_TYPE_LONG:
            *(unsigned long*)&data[0] = *(unsigned long*)arg1;
            break;
        case MYSQL_TYPE_FLOAT:
            *(float*)&data[0] = *(float*)arg1;
            break;
        case MYSQL_TYPE_STRING:
            buf_len[j] = (uint32)strlen((char*)arg1);
            bufs[j] = (char*)arg1;
            *(uint32*)&data[0] = buf_len[j];
            total_buf_len += ++buf_len[j];
            j++;
            break;
    }

    for(i = 1; i < format_len; i++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
                buf_len[j] = va_arg(ap, uint32);
                bufs[j] = va_arg(ap, char*);
                *(uint32*)&data[i] = buf_len[j];
                total_buf_len += buf_len[j];
                j++;
                break;
            case MYSQL_TYPE_TINY:
                *(char*)&data[i] = va_arg(ap, char);
                break;
            case MYSQL_TYPE_LONG:
                *(unsigned long*)&data[i] = va_arg(ap, unsigned long);
                break;
            case MYSQL_TYPE_FLOAT:
                *(float*)&data[i] = va_arg(ap, float);
                break;
            case MYSQL_TYPE_STRING:
                aux = va_arg(ap, char*);
                buf_len[j] = (uint32)strlen(aux);
                bufs[j] = aux;
                *(uint32*)&data[i] = buf_len[j];
                total_buf_len += ++buf_len[j];
                j++;
                break;
        }
    }

    char *buf;
    if(nr_strings > 0)
    {
        buf = (char*)malloc(total_buf_len);

        j = 0;
        for(i = 0; i < nr_strings; i++)
        {
            memcpy(&buf[j], bufs[i], buf_len[i]);
            bufs[i] = &buf[j];
            j += buf_len[i];
        }
    }

    Execute(raw_data);
}

void MySQLPreparedStatement::_DirectPExecute(void *arg1, va_list ap)
{
    char *aux;
    int i, j = 0;
    switch(format[0])
    {
        case MYSQL_TYPE_BLOB:
            *(uint32*)&m_data[0] = *(uint32*)arg1;
            m_bind[m_str_idx[j++]].buffer = va_arg(ap, char*);
            break;
        case MYSQL_TYPE_TINY:
            *(char*)&m_data[0] = *(char*)arg1;
            break;
        case MYSQL_TYPE_LONG:
            *(unsigned long*)&m_data[0] = *(unsigned long*)arg1;
            break;
        case MYSQL_TYPE_FLOAT:
            *(float*)&m_data[0] = *(float*)arg1;
            break;
        case MYSQL_TYPE_STRING:
            *(uint32*)&m_data[0] = (uint32)strlen((char*)arg1);
            m_bind[m_str_idx[j++]].buffer = *(char**)arg1;
            break;
    }

    for(i = 1; i < format_len; i++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
                *(uint32*)&m_data[i] = va_arg(ap, uint32);
                m_bind[m_str_idx[j++]].buffer = va_arg(ap, char*);
                break;
            case MYSQL_TYPE_TINY:
                *(char*)&m_data[i] = va_arg(ap, char);
                break;
            case MYSQL_TYPE_LONG:
                *(unsigned long*)&m_data[i] = va_arg(ap, unsigned long);
                break;
            case MYSQL_TYPE_FLOAT:
                *(float*)&m_data[i] = va_arg(ap, float);
                break;
            case MYSQL_TYPE_STRING:
                aux = va_arg(ap, char*);
                *(uint32*)&m_data[i] = (uint32)strlen(aux);
                m_bind[m_str_idx[j++]].buffer = aux;
                break;
        }
    }

    DirectExecute();
}

QueryResult * MySQLPreparedStatement::_PQuery(void *arg1, va_list ap)
{
    return NULL;
}

#endif
