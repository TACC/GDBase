/* This file is part of GDBase.
 *
 * GDBase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GDBase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GDBase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Anthony DiGirolamo, Dan Stanzione, Karl Lindekugel
 */

#ifndef DBREPORT_H
#define DBREPORT_H
/*
 * Reports strings to the Database for OPD
 * Karl Lindekugel, ASU
 */
#include <libpq-fe.h>


typedef struct struct_DB
{
  PGconn *conn;
  PGresult *res;
  int jobid;
} DB;

void db_closeConnection(DB *d);
DB* db_openConnection();
void db_startJob(DB *d, char *jobname, char *jobnum, int tasknum);
void db_endJob(DB *d, int tasknum);

void db_logSysMessage(DB *d, int tasknum, char *msg);
void db_logStdoutMessage(DB *d, int tasknum, char *msg);
void db_logStderrMessage(DB *d, int tasknum, char *msg);
void db_logGdbmiMessage(DB *d, int tasknum, char *msg);
void db_logOpdMessage(DB *d, int tasknum, char *msg);

#endif
