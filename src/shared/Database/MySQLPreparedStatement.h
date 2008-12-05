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

#ifndef _MYSQL_PREPARED_STATEMENT_H_
#define _MYSQL_PREPARED_STATEMENT_H_

#ifdef WIN32
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

#include "Platform/Define.h"
#include "Database/PreparedStatement.h"

class DatabaseMysql;

class MySQLPreparedStatement : public PreparedStatementBase< MySQLPreparedStatement >
{
    friend class MySQLPreparedStatementBinder;
    public:
        MySQLPreparedStatement(DatabaseMysql *db, const char *sql, va_list ap);
        ~MySQLPreparedStatement();
        
        bool DirectExecute();
        bool Execute();
        QueryResult * Query();

        bool Execute(char *raw_data);
        bool DirectExecute(char *raw_data);
        void Free(char *raw_data);
    private:
        bool _DirectPExecute(void *arg1, va_list ap);
        bool _PExecute(void *arg1, va_list ap);
        QueryResult * _PQuery(void *arg1, va_list ap);

        static void _set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len);

        DatabaseMysql *m_db;

        MYSQL_STMT * m_stmt;
        MYSQL_BIND * m_bind;

        enum_field_types * format;
        int format_len;
        
        uint64 * m_data;

        char ** m_bufs;
        int * m_str_idx;
        int nr_strings;
};

class MySQLPreparedStatementBinder : public PreparedStatementBinderBase<MySQLPreparedStatementBinder>
{
    public:
        MySQLPreparedStatementBinder(MySQLPreparedStatement *stmt)
            : m_stmt(stmt), m_poz(0), m_str_poz(0), m_total_buf_len(0)
        {
            m_bind = new MYSQL_BIND[m_stmt->format_len];
            memset(m_bind, 0, sizeof(m_bind));
            // the other arrays are always overwritten
            m_data = (uint64*)new char[m_stmt->format_len*sizeof(uint64)+m_stmt->nr_strings*sizeof(char*)];
            m_bufs = (char**)&m_data[m_stmt->format_len];
        }

        void append(unsigned long x)
        {
            *(unsigned long*)&m_data[m_poz] = x;
            m_poz++;
        }

        void append(float x)
        {
            *(float*)&m_data[m_poz] = x;
            m_poz++;
        }

        void append(char x)
        {
            *(char*)&m_data[m_poz] = x;
            m_poz++;
        }

        void append(uint64 x)
        {
            *(uint64*)&m_data[m_poz] = x;
            m_poz++;
        }

        void append(char *str)
        {
            uint32 len = strlen(str);
            *(uint32*)&m_data[m_poz] = len;
            m_bufs[m_str_poz] = str;
            m_total_buf_len += len;
            m_poz++, m_str_poz++;
        }

        void append(unsigned long len, char *buf)
        {
            *(uint32*)&m_data[m_poz] = len;
            m_bufs[m_str_poz] = buf;
            m_total_buf_len += len;
            m_poz++, m_str_poz++;
        }

        void Execute()
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
                    uint32 len = *(uint32*)&(m_stmt->m_data[format_idx]);
                    // one extra byte for the terminating '\0'
                    if(m_stmt->format[format_idx] == MYSQL_TYPE_STRING)
                        len++;

                    memcpy(&buf[j], m_bufs[i], len);
                    m_bufs[i] = &buf[j];
                    j += len;
                }
            }

            m_stmt->DirectExecute((char*)m_data);
        }

    private:

        MySQLPreparedStatement * m_stmt;
        MYSQL_BIND *m_bind;
        uint64 *m_data;
        char **m_bufs;
        uint32 m_poz;
        uint32 m_str_poz;
        uint32 m_total_buf_len;
};

#endif
