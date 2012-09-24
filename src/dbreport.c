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

/*
 * Reports strings to the Database for OPD
 * Karl Lindekugel, ASU
 */

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "dbreport.h"

#define TYPE_SYS 1
#define TYPE_STDERR 2
#define TYPE_STDOUT 3
#define TYPE_GDBMI 4
#define TYPE_OPDOUT 5

void db_closeConnection(DB *d) 
{
  PQfinish(d->conn);
  free(d);
}

DB* db_openConnection() 
{
  DB* d;
  d = (DB*) malloc( sizeof( DB ) );
  d->conn = PQconnectdb(" dbname=OPD host=hpcdb.local port=5432 user=opd password=opd123 "); //TODO: Make this easily configured
        
  if ( PQstatus(d->conn) != CONNECTION_OK) 
    {
      fprintf(stderr, " Error connecting to database: %s", PQerrorMessage(d->conn));
      db_closeConnection(d);
    }
  d->jobid= -1;


  return d;
}

void db_startJob(DB *d, char *jobname, char *jobnum, int tasknum) 
{
  /* If we're rank 0, insert */
  if (!tasknum) 
    {
      // Insert into DB those values
      char querystr[] = "INSERT INTO jobs ( sched_number, sched_name, cluster ) VALUES ('%s', '%s', 'saguaro'  );";
      int limit = sizeof(querystr) + 2*128 + 1;
      char *query = (char *) malloc( sizeof(char) * limit );
      snprintf( query, limit, querystr, jobnum, jobname);
      
      d->res = PQexec(d->conn, query); //Execute Query
      if ( PQresultStatus(d->res) != PGRES_COMMAND_OK ) 
        {
          fprintf(stderr, "Error starting job: %s", PQerrorMessage(d->conn));
          PQclear(d->res);
          db_closeConnection(d);
        }
      PQclear(d->res);
    }

  //Now everyone find the jobid for the task
  do
    {
      //Do until this works
      char querystr[] = "SELECT * FROM jobs WHERE cluster = 'saguaro' AND sched_number = '%s';";
      int  limit = sizeof(querystr) + 1*128 + 1;
      char *query = (char *) malloc( sizeof(char) * limit );
      snprintf( query, limit, querystr, jobnum );
      
      d->res = PQexec(d->conn, query);
      if ( PQresultStatus(d->res) != PGRES_COMMAND_OK && PQresultStatus(d->res) != PGRES_TUPLES_OK ) 
        {
          fprintf(stderr, "Error starting job, finding id: %s", PQerrorMessage(d->conn));
          PQclear(d->res);
          db_closeConnection(d);
          exit(0);
        }
      if ( PQntuples(d->res)  > 0 ) 
        {
          //Extract JobId
          int colnum = PQfnumber(d->res, "id");
          d->jobid = atoi( PQgetvalue(d->res, 0, colnum) );
        }
      else
        {
          // Sleep for .1 seconds
          sleep(1);
        }
      
      PQclear(d->res);
    }
  while ( d->jobid < 0 );


  // Log message about staring the job!
  db_logSysMessage(d, tasknum, "Started Debugger on this task");

}

void db_endJob(DB *d, int tasknum) 
{
  if (!tasknum)
    {
      //Tasknum = 0
      char querystr[] = "UPDATE jobs SET end_time = NOW() WHERE id = %d;";
      int limit = sizeof(querystr) + 128 + 1;
      char *query = (char*) malloc( sizeof(char) * limit);
      snprintf(query, limit, querystr, d->jobid);

      d->res = PQexec(d->conn, query);
      if ( PQresultStatus(d->res) != PGRES_COMMAND_OK ) 
        {
          fprintf(stderr, "Error ending job, setting time: %s", PQerrorMessage(d->conn));
          PQclear(d->res);
          db_closeConnection(d);
          exit(1);
        }
      PQclear(d->res);
    }
}

void db_logMessage(DB *d, int tasknum, int type, char *msg) 
{
  char querystr[] = "INSERT INTO messages ( jobid, msg_type, tasknum, msg ) VALUES ( %d, %d, %d, '%s' );";
  size_t limit    = strlen(msg)*2;
  char *es_msg    = (char*) malloc(sizeof(char) * limit);
  limit           = PQescapeStringConn(d->conn, es_msg, msg, strlen(msg), NULL); //Returns bytes written to es_msg
  limit           = sizeof(querystr) + 3*sizeof(int) + limit;  //Set size of insert
  char *qry       = (char*) malloc(sizeof(char) * limit + 1);
  
  snprintf(qry, limit, querystr, d->jobid, type, tasknum, es_msg);

  d->res = PQexec(d->conn, qry);
  if ( PQresultStatus(d->res) != PGRES_COMMAND_OK ) 
    {
      fprintf(stderr, "Error ending job, setting time: %s", PQerrorMessage(d->conn));
      PQclear(d->res);
      db_closeConnection(d);
      exit(1);
    }
  PQclear(d->res);
  free(qry);
  free(es_msg);
}

void db_logSysMessage(DB *d, int tasknum, char *msg) 
{
  db_logMessage(d,tasknum,TYPE_SYS, msg);
}

void db_logStdoutMessage(DB *d, int tasknum, char *msg) 
{
  db_logMessage(d,tasknum,TYPE_STDOUT,msg);
}

void db_logStderrMessage(DB *d, int tasknum, char *msg) 
{
  db_logMessage(d,tasknum,TYPE_STDERR, msg);
}

void db_logGdbmiMessage(DB *d, int tasknum, char *msg) 
{
  db_logMessage(d,tasknum,TYPE_GDBMI, msg);
}

void db_logOpdMessage(DB *d, int tasknum, char *msg) 
{
  db_logMessage(d,tasknum,TYPE_OPDOUT, msg);
}

#if 0

int main(int argc, char* argv[])
{
  DB *d;
  d = db_openConnection();
  db_startJob(d, "test.job", "1234.local", 0);
  db_endJob(d, 0);
  db_closeConnection(d);
  
  return 0;
}
#endif
