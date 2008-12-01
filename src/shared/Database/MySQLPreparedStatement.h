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
    template< uint32 N > friend class MySQLPreparedStatementBinder;
    public:
        MySQLPreparedStatement(DatabaseMysql *db, const char *sql, va_list ap);
        ~MySQLPreparedStatement();
        
        void DirectExecute();
        void Execute();
        QueryResult * Query();

        void DirectExecute(MYSQL_BIND *binds);
    private:
        void _DirectPExecute(void *arg1, va_list ap);
        void _PExecute(void *arg1, va_list ap);
        QueryResult * _PQuery(void *arg1, va_list ap);

        static void _set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len);

        MYSQL_STMT * m_stmt;
        enum_field_types * format;
        int format_len;
        int nr_blobs;
        int nr_strings;
        MYSQL_BIND * m_bind;
        uint64 * m_data;
        int * m_str_idx;
};

template< uint32 N >
class MySQLPreparedStatementBinder : public PreparedStatementBinderBase< MySQLPreparedStatementBinder<N> >
{
    public:
        MySQLPreparedStatementBinder(MySQLPreparedStatement *stmt)
            : m_stmt(stmt), m_poz(0)
        {
            memset(m_bind, 0, sizeof(m_bind));
            // the other arrays are always overwritten
        }

        void append(unsigned long x)
        {
            *(unsigned long*)&m_data[m_poz] = x;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_INTEGER, (char*)&m_data[m_poz], 0, NULL);
            m_poz++;
        }

        void append(float x)
        {
            *(float*)&m_data[m_poz] = x;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_FLOAT, (char*)&m_data[m_poz], 0, NULL);
            m_poz++;
        }

        void append(char x)
        {
            *(char*)&m_data[m_poz] = x;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_TINYINT, (char*)&m_data[m_poz], 0, NULL);
            m_poz++;
        }

        void append(uint64 x)
        {
            *(uint64*)&m_data[m_poz] = x;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_BIGINT, (char*)&m_data[m_poz], 0, NULL);
            m_poz++;
        }

        void append(char *str)
        {
            m_length[m_poz] = strlen(str);
            *(char *)&m_data[m_poz] = str;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_STRING, str, m_length[m_poz] + 1, &m_length[m_poz]);
            m_poz++;
        }

        void append(unsigned long len, char *buf)
        {
            m_length[m_poz] = len;
            *(char *)&m_data[m_poz] = buf;
            MySQLPreparedStatement::_set_bind(m_bind[m_poz], MYSQL_TYPE_BLOB, buf, m_length[m_poz], &m_length[m_poz]);
            m_poz++;
        }

        void Execute()
        {
            m_stmt->DirectExecute(m_bind);
        }

    private:

        MySQLPreparedStatement * m_stmt;
        MYSQL_BIND m_bind[N];
        unsigned long m_length[N];
        uint64 m_data[N];
        uint32 m_poz;
};

#endif
