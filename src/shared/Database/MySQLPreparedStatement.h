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
    friend class MySQLPreparedStatementDirectBinder;
    public:
        MySQLPreparedStatement(DatabaseMysql *db, const char *sql, va_list ap);
        ~MySQLPreparedStatement();
        
        bool DirectExecute();
        bool Execute();
        QueryResult * Query();

        bool Execute(char *raw_data);
        bool DirectExecute(char *raw_data);

        void Free(char *raw_data);

        bool _DirectPExecute(void *arg1, va_list ap);
        bool _PExecute(void *arg1, va_list ap);
        QueryResult * _PQuery(void *arg1, va_list ap);
    private:

        static void _set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len);
        template< class B >
            void _parse_args(B &binder, void *arg1, va_list ap);

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

class MySQLPreparedStatementBinder : public PreparedStatementAsyncBinderBase<MySQLPreparedStatementBinder>
{
    template< class D > friend class PreparedStatementBase;
    public:
        MySQLPreparedStatementBinder(MySQLPreparedStatement *stmt);

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

        void append(double x)
        {
            *(double*)&m_data[m_poz] = x;
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
            m_total_buf_len += len+1;
            m_poz++, m_str_poz++;
        }

        void append(unsigned long len, char *buf)
        {
            *(uint32*)&m_data[m_poz] = len;
            m_bufs[m_str_poz] = buf;
            m_total_buf_len += len;
            m_poz++, m_str_poz++;
        }

        bool Execute();

    private:

        MySQLPreparedStatement * m_stmt;
        uint64 *m_data;
        char **m_bufs;
        uint32 m_poz;
        uint32 m_str_poz;
        uint32 m_total_buf_len;
};

class MySQLPreparedStatementDirectBinder : public PreparedStatementDirectBinderBase<MySQLPreparedStatementDirectBinder>
{
    template< class D > friend class PreparedStatementBase;
    public:
        MySQLPreparedStatementDirectBinder(MySQLPreparedStatement *stmt);

        void append(unsigned long x)
        {
            *(unsigned long*)&m_stmt->m_data[m_poz] = x;
            m_poz++;
        }

        void append(float x)
        {
            *(float*)&m_stmt->m_data[m_poz] = x;
            m_poz++;
        }

        void append(double x)
        {
            *(double*)&m_stmt->m_data[m_poz] = x;
            m_poz++;
        }

        void append(char x)
        {
            *(char*)&m_stmt->m_data[m_poz] = x;
            m_poz++;
        }

        void append(uint64 x)
        {
            *(uint64*)&m_stmt->m_data[m_poz] = x;
            m_poz++;
        }

        void append(char *str)
        {
            uint32 len = strlen(str);
            *(uint32*)&m_stmt->m_data[m_poz] = len;
            memcpy(m_stmt->m_bufs[m_str_poz], str, len+1);
            m_poz++, m_str_poz++;
        }

        void append(unsigned long len, char *buf)
        {
            *(uint32*)&m_stmt->m_data[m_poz] = len;
            memcpy(m_stmt->m_bufs[m_str_poz], buf, len);
            m_poz++, m_str_poz++;
        }

        bool DirectExecute();

    private:

        MySQLPreparedStatement * m_stmt;
        uint32 m_poz;
        uint32 m_str_poz;
};

#endif
