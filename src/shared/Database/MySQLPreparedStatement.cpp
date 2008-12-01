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

#include "Database/MySQLPreparedStatement.h"
#include "Database/DatabaseEnv.h"

MySQLPreparedStatement::MySQLPreparedStatement(DatabaseMysql *db, const char *sql)
{
    m_stmt = mysql_stmt_init(db->mMysql);
    if(!m_stmt)
    {
        sLog.outError("mysql_stmt_init(), out of memory");
        assert(false);
    }

    size_t sql_len = strlen(sql);
    char *stmt_str = new char[sql_len+1], *p, *q;

    format = (enum_field_types*)malloc(sql_len*sizeof(enum_field_types));
    format_len = 0;
    
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

    if(mysql_stmt_prepare(m_stmt, stmt_str, q - stmt_str))
    {
        sLog.outError("mysql_stmt_prepare(), %s", mysql_stmt_error(m_stmt));
        assert(false);
    }

    delete[] stmt_str;
    if(format_len == 0)
    {
        free(format);
        format = NULL;
    }
    else
        format = (enum_field_types*)realloc(format, format_len*sizeof(enum_field_types));
}

MySQLPreparedStatement::~MySQLPreparedStatement()
{
    free(format);
    if(mysql_stmt_close(m_stmt))
        sLog.outError("failed while closing the prepared statement");
}

void MySQLPreparedStatement::Execute()
{

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
    MYSQL_BIND bind[2048];
    unsigned long length[2048];
    char *aux;

    memset(bind, 0, format_len * sizeof(MYSQL_BIND));

    switch(format[0])
    {
        case MYSQL_TYPE_BLOB:
            _set_bind(bind[0], format[0], va_arg(ap, char *), *(unsigned long*)arg1, (unsigned long*)arg1);                         
            break;
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
            _set_bind(bind[0], format[0], (char *)arg1, 0, NULL);
            break;
        case MYSQL_TYPE_STRING:
            length[0] = strlen((char*)arg1);
            _set_bind(bind[0], format[0], (char *)arg1, length[0]+1, &length[0]);
            break;
    }

    for(int i = 1; i < format_len; i++)
    {
        switch(format[i])
        {
            case MYSQL_TYPE_BLOB:
                length[i] = va_arg(ap, unsigned long);
                _set_bind(bind[i], format[i], va_arg(ap, char *), length[i], &length[i]);
                break;
            case MYSQL_TYPE_TINY:
                // note: the standard doesn't say va_arg has to be an lvalue
                // but since it is on most platforms, it's better to keep it in this form
                // for performance reasons (one less copy)
                _set_bind(bind[i], format[i], (char*)&va_arg(ap, char), 0, NULL);
                break;
            case MYSQL_TYPE_LONG:
                _set_bind(bind[i], format[i], (char*)&va_arg(ap, unsigned long), 0, NULL);
                break;
            case MYSQL_TYPE_FLOAT:
                _set_bind(bind[i], format[i], (char*)&va_arg(ap, float), 0, NULL);
                break;
            case MYSQL_TYPE_STRING:
                aux = va_arg(ap, char*);
                length[i] = strlen(aux);
                _set_bind(bind[i], format[i], aux, length[i]+1, &length[i]);
                break;
        }
    }

    if(mysql_stmt_bind_param(m_stmt, bind))
    {
        sLog.outError("mysql_stmt_bind_param() failed, %s", mysql_stmt_error(m_stmt));
        assert(false);
    }

    if(mysql_stmt_execute(m_stmt))
    {
        // can this occur on query syntax error ?
        sLog.outError("mysql_stmt_execute() failed, %s", mysql_stmt_error(m_stmt));
        assert(false);
    }
}

QueryResult * MySQLPreparedStatement::_PQuery(void *arg1, va_list ap)
{
    return NULL;
}

