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

#ifndef _PREPARED_STATEMENT_H_
#define _PREPARED_STATEMENT_H_

#include <cstdarg>

class QueryResult;

class PreparedStatement
{
    public:
        PreparedStatement() {}
        virtual ~PreparedStatement() {}
        
        virtual void Execute() = 0;
        virtual QueryResult * Query() = 0;

        // used as: void PExecute(...);
        template<class T> void PExecute(T arg1, ...)
            { va_list ap; va_start(ap, arg1); _PExecute((void*)&arg1, ap); va_end(ap); }
        // used as: QueryResult * PQuery(...);
        template<class T> QueryResult * PQuery(T arg1, ...)
            { va_list ap; va_start(ap, arg1); QueryResult * ret = _PQuery((void*)&arg1, ap); va_end(ap); return ret; }
    private:
        virtual void _PExecute(void *arg1, va_list ap);
        virtual QueryResult * _PQuery(void *arg1, va_list ap);
};

#endif
