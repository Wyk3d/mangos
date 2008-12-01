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

#include "Database/PreparedStatement.h"

#ifdef WIN32
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

class DatabaseMysql;

class MySQLPreparedStatement : public PreparedStatement
{
    public:
        MySQLPreparedStatement(DatabaseMysql *db, const char *sql);
        ~MySQLPreparedStatement();
        
        void Execute();
        QueryResult * Query();
    private:
        void _PExecute(void *arg1, va_list ap);
        QueryResult * _PQuery(void *arg1, va_list ap);

        void _set_bind(MYSQL_BIND &bind, enum_field_types type, char *value, unsigned long buf_len, unsigned long *len);

        MYSQL_STMT * m_stmt;
        enum_field_types *format;
        size_t format_len;
};

#endif
