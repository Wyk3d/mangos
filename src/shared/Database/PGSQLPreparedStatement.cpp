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

#ifdef DO_POSTGRESQL

PGSQLPreparedStatement::PGSQLPreparedStatement(DatabasePostgre *db, const char *sql)
{
	ZThread::Guard<ZThread::FastMutex> query_connection_guard(db->mMutex);
	m_db = db;

	// generate a unique statement name
	char buf[1024];
	snprintf(buf, 1024, "s%d", m_db->preparedCounter++);
	int len = (int)strnlen(buf, 1024);
	stmtName = new char[len+1];
	memcpy(stmtName, buf, len+1);

	// do the prepare
	PQprepare(m_db->mPGconn, stmtName, sql, format_len, format);
}

bool PGSQLPreparedStatement::Execute()
{
	return true;
}

QueryResult * PGSQLPreparedStatement::Query()
{
    return NULL;
}

bool PGSQLPreparedStatement::_PExecute(void *arg1, va_list ap)
{
	return true;
}

QueryResult * PGSQLPreparedStatement::_PQuery(void *arg1, va_list ap)
{
    return NULL;
}

#endif
