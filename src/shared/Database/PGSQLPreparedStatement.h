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

#ifndef _PGSQL_PREPARED_STATEMENT_H_
#define _PGSQL_PREPARED_STATEMENT_H_

#include "Database/PreparedStatement.h"

class DatabasePostgre;

class PGSQLPreparedStatement : public PreparedStatementBase< PGSQLPreparedStatement >
{
    public:
        PGSQLPreparedStatement(DatabasePostgre *db, const char *sql);
        
        void Execute();
        QueryResult * Query();
    private:
        void _PExecute(void *arg1, va_list ap);
        QueryResult * _PQuery(void *arg1, va_list ap);

        DatabasePostgre *m_db;
};

template< uint32 N >
class PGSQLPreparedStatementBinder : public PreparedStatementBinderBase< PGSQLPreparedStatementBinder<N> >
{
    public:
        PGSQLPreparedStatementBinder(PGSQLPreparedStatement *stmt)
            : m_stmt(stmt)
        {
        }

        void append(unsigned long x)
        {
        }

        void append(float x)
        {
        }

        void append(char x)
        {
        }

        void append(uint64 x)
        {
        }

        void append(char *str)
        {
        }

        void append(unsigned long len, char *buf)
        {
        }

        void Execute()
        {
        }

    private:
        PGSQLPreparedStatement * m_stmt;
};

#endif
