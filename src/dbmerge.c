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

//  Merges individual SQLite databases into single Postgres DB
//  - Karl Lindekugel, ASU

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <libpq-fe.h>
#include <sqlite3.h>

#include "parser.h"
#define malloc_check(var) if(var==NULL){fprintf(stderr,"malloc failed\n");exit(EXIT_FAILURE);}

const char * mpi_job_names [] =
{
  "PBS_JOBNAME",
  "JOB_NAME",
  0
};

const char * mpi_job_id [] =
{
  "PBS_JOBID",
  "JOB_ID",
  0
};



int exists(const char *filename) 
{
  return !access(filename, F_OK);
}

void printhelp() 
{
  printf("Usage: dbmerge [OPTIONS] [EXECNAME]\n");
  printf("Options: \n");
  printf("  -h, --help        Display this message\n");
  printf("  -c, --config      Database config file\n");
  printf("  -j, --jobid       Custom job id string for database logging\n");
  printf("  -J  --jobid_var   Environment variable for job id\n");
  printf("                     Default: PBS_JOBID\n");
  printf("  -n, --jobname     Custom job name string for database logging\n");
  printf("  -N  --jobname_var Environment variable for job name\n");
  printf("                     Default: PBS_JOBNAME\n");
}

int main(int argc, char* argv[]) 
{
  const char **arrayp;
  char *temp = NULL;
  char *jobid = NULL;
  char *jobid_var = NULL;
  char *jobname = NULL;
  char *jobname_var = NULL;
  char *config_file = "db.config";
  char *database = NULL;
  
  int jobnum;
  char s_jobnum[256];
  
  int rank;
  PGconn *conn;
  PGresult *res;
  const char *values[3];
  char strbuf[4096];
  
  FILE *file_pointer;
  int line_size = 1024;
  int line_length;
  char *buffer;
  
  int c = 0;
  int target_index = 0;
  
  while (c != -1) 
    {
      static struct option long_options[] = 
        {
          {"help",	  no_argument,       0, 'h'},
          {"config",	  required_argument, 0, 'c'},
          {"jobid",	  required_argument, 0, 'j'},
          {"jobid_var",	  required_argument, 0, 'J'},
          {"jobname",	  required_argument, 0, 'n'},
          {"jobname_var", required_argument, 0, 'N'},
          {0, 0, 0, 0}
        };
      int option_index = 0;
      c = getopt_long (argc, argv, "hc:j:J:n:N:", long_options, &option_index);
      if (c == -1) break;

      switch (c) 
        {
        case 0:
          // If this option set a flag, do nothing else now.
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;
        case 'h':
          printhelp();
          return EXIT_SUCCESS;
          break;
        case 'c':
          config_file = optarg;
          break;
        case 'j':
          jobid = optarg;
          break;
        case 'J':
          jobid_var = optarg;
          break;
        case 'n':
          jobname = optarg;
          break;
        case 'N':
          jobname_var = optarg;
          break;
        default:
          abort();
        }
    }

  //check to see we got exec name as parameter
  if (optind < argc) 
    target_index = optind;
  else 
    {
      fprintf(stderr, "Missing application name. Try -h for help.\n");
      return EXIT_SUCCESS;
    }

  rank = 0;

  //Get Jobname
  if (!jobname) 
    {
      jobname = "DEFAULTNAME";
      // check for a custom env var
      if (jobname_var) 
        {
          if (getenv(jobname_var))
            jobname = getenv(jobname_var);
        }
      else // check for MPI Queue job names
	{
	  for (arrayp = mpi_job_names; *arrayp; ++arrayp)
	    {
	      if (getenv(*arrayp))
		{
		  jobname = getenv(*arrayp);
		  break;
		}
	    }
	}
    }

  // Get Jobid
  if (!jobid) 
    {
      jobid = "DEFAULTID";
      // check for a custom env var
      if (jobid_var) 
        {
          if (getenv(jobid_var))
            jobid = getenv(jobid_var);
        } 
      else // check for MPI Queue job id
	{
	  for (arrayp = mpi_job_id; *arrayp; ++arrayp)
	    {
	      if (getenv(*arrayp))
		{
		  jobid = getenv(*arrayp);
		  break;
		}
	    }
	}
    }

  if (!exists(config_file)) 
    {
      fprintf(stderr, "cannot access config file %s\n", config_file);
      return EXIT_SUCCESS;
    }
  else
    {
      //parse( (char*) config_file );
      //database = get_connectdb();
      //printf("%s\n", database);
      //return EXIT_SUCCESS;
      
      buffer = (char *) malloc(line_size * sizeof(char));
      malloc_check(buffer);
      
      // Open the file
      if ((file_pointer = fopen(config_file, "r")) == NULL ) 
        {
          fprintf(stderr, "can't open %s\n", config_file);
          return EXIT_FAILURE;
        }

      line_length = getline(&buffer, &line_size, file_pointer);
      //printf("%s\n", buffer);
      fclose(file_pointer);
    }

  //Connect to Postgres database
  // conn = PQconnectdb(" dbname=OPD host=localhost port=5432 user=opd password=opd123 ");
  conn = PQconnectdb(buffer);
  
  if ( PQstatus(conn) != CONNECTION_OK) 
    {
      fprintf(stderr, " Error connecting to database: %s", PQerrorMessage(conn));
      PQfinish(conn);
      return 1;
    }

  //Begin transaction
  res = PQexec(conn, "BEGIN;");
  if ( PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
      fprintf(stderr, "Error starting transaction, msg: %s", PQerrorMessage(conn));
      PQclear(res);
      PQfinish(conn);
      return 1;
    }
  PQclear(res);

  //Create Job, save id for later
  values[0] = jobname;
  values[1] = jobid;
  values[2] = argv[target_index];
  res = PQexecParams(conn, "INSERT INTO jobs (job_name, queueid, exec_name) VALUES ($1, $2, $3 );", 3, NULL, values, NULL, NULL, 0);
  if ( PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
      fprintf(stderr, "Error creating job, msg: %s", PQerrorMessage(conn));
      PQclear(res);
      PQfinish(conn);
      return 1;
    }
  PQclear(res);

  //obtain job id num
  
  res = PQexec(conn, "SELECT currval('jobs_id_seq');");
  if ( PQresultStatus(res) != PGRES_COMMAND_OK  && PQresultStatus(res) != PGRES_TUPLES_OK ) 
    {
      fprintf(stderr, "Error obtaining jobid, msg: %s", PQerrorMessage(conn));
      PQclear(res);
      PQfinish(conn);
      return 1;
    }

  if ( PQntuples(res)  > 0 ) 
    {
      //Extract JobId
      jobnum = atoi(PQgetvalue(res, 0, 0) );
      sprintf(s_jobnum, "%d", jobnum);
    }
  PQclear(res);

  sprintf(strbuf, "%s-%s.%d", "gdblog", argv[target_index], rank);
  //if a database exists
  while (exists(strbuf)) 
    {
      sqlite3 *db;
      char *query;
      int ret;
      char *zErrMsg = NULL;
      query = NULL;
      sqlite3_stmt* stmt;

      //open database
      ret = sqlite3_open(strbuf, &db);
      //Check ret
      if ( ret ) 
        {
          printf("Can't open database %s: %s\n", strbuf, sqlite3_errmsg(db));
          sqlite3_close(db);
          PQfinish(conn);
          return 1;
        }

      //create cursor

      sqlite3_prepare(db, "SELECT * FROM messages order by id asc", -1, &stmt, NULL);

      while ( (ret = sqlite3_step(stmt)) == SQLITE_ROW) 
        {
          const char *pgvalues[5];
          char sbuf[256];
          int i;
          //read row from SqlLite DB
          for (i = 1; i < 4; i++)
            pgvalues[i+1] = sqlite3_column_text(stmt, i);
          
          pgvalues[0] = s_jobnum;
          sprintf(sbuf, "%d", rank);
          pgvalues[1] = sbuf;
          
          //write row to PostgreSQL DB
          res = PQexecParams(conn, "INSERT INTO messages (job_id, rank,tstamp, key, value) "
                                   "VALUES (($1::text)::integer, ($2::text)::integer, ($3::text)::timestamp, $4, $5);",
                             5, NULL, pgvalues, NULL, NULL, 0);
          if ( PQresultStatus(res) != PGRES_COMMAND_OK) 
            {
              fprintf(stderr, "Error inserting row, msg: %s", PQerrorMessage(conn));
              sqlite3_close(db);
              PQclear(res);
              PQfinish(conn);
              return 1;
            }
          PQclear(res);
          
        }

      //Close Statement
      ret = sqlite3_finalize(stmt);
      if (0&& ret != SQLITE_OK ) 
        {
          printf("Can't finalize statement %d: %s\n",ret, sqlite3_errmsg(db));
          PQfinish(conn);
          return 1;
        }

      //close database
      ret = sqlite3_close(db);
      if ( ret != SQLITE_OK ) 
        {
          printf("Can't close database %s: %s\n", strbuf, sqlite3_errmsg(db));
          PQfinish(conn);
          return 1;
        }


      //Go to next database
      rank++;
      sprintf(strbuf, "%s-%s.%d", "gdblog", argv[target_index], rank);
    }

  //Update with start, end times

  //Commit Transaction
  res = PQexec(conn, "COMMIT;");
  if ( PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
      fprintf(stderr, "Error commiting transaction, msg: %s", PQerrorMessage(conn));
      PQclear(res);
      PQfinish(conn);
      return 1;
    }
  PQclear(res);

  //close DB connection
  PQfinish(conn);
  
  return 0;
}

