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

    format = (char*)malloc(sql_len);
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
                        format[format_len++] = (char)MYSQL_TYPE_BLOB;
                        break;
                    case 'c':
                        format[format_len++] = (char)MYSQL_TYPE_TINY;
                        break;
                    case 'd':
                    case 'i':
                    case 'u':
                        format[format_len++] = (char)MYSQL_TYPE_LONG;
                        break;
                    case 'f':
                        format[format_len++] = (char)MYSQL_TYPE_FLOAT;
                        break;
                    case 's':
                        format[format_len++] = (char)MYSQL_TYPE_STRING;
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
        format = (char*)realloc(format, format_len);
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

void MySQLPreparedStatement::_PExecute(void *arg1, va_list ap)
{

}

QueryResult * MySQLPreparedStatement::_PQuery(void *arg1, va_list ap)
{
    return NULL;
}
