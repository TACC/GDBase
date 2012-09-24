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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <tcl.h>

#include "dblog.h"

sqlite3 *db;
char *query;

int db_open(char *name) 
{
  int ret;
  char *zErrMsg = NULL;
  query = NULL;
  
  //NEED TO CLEANUP NAME incase it is screwed up!

  ret = sqlite3_open(name, &db);
  //Check ret
  if ( ret ) 
    {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return 1;
    }



  //Create table for logging messages
  ret = sqlite3_exec(db, "CREATE TABLE messages(id INTEGER PRIMARY KEY AUTOINCREMENT, time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, key TEXT , value TEXT);",
                     NULL, 0, &zErrMsg);
  if ( ret != SQLITE_OK ) 
    {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return 2;
    }

  ret = sqlite3_exec(db, "PRAGMA locking_mode = EXCLUSIVE;", NULL, 0, &zErrMsg);
  if ( ret != SQLITE_OK ) 
    {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return 3;
    }

  ret = sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, 0, &zErrMsg);
  if ( ret != SQLITE_OK ) 
    {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return 3;
    }

  return 0;
}

int db_close() 
{
  free(query);
  return  sqlite3_close(db);
}

int db_tcl_logMessage(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
  Tcl_Obj *retObj;
  int strlen = 0;
  int ret;
  char *key;
  char *val;
  
  if (objc!=3) 
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  key = Tcl_GetStringFromObj(objv[1], &strlen);
  val = Tcl_GetStringFromObj(objv[2], &strlen);
  
  ret = db_logMessage(key, val);
  
  retObj = Tcl_GetObjResult(interp);
  Tcl_SetIntObj(retObj, ret);
  
  Tcl_SetObjResult(interp, retObj);
  
  return TCL_OK;
}


int db_logMessage(char *key, char *val) 
{
  int ret;
  sqlite3_stmt* stmt;
  
  if ( !strlen(key) || !strlen(val) ) return 0;
  
  sqlite3_prepare(db, "INSERT INTO messages (key, value) VALUES (?,?);", -1, &stmt, NULL);
  
  sqlite3_bind_text(stmt,1,key, strlen(key), SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, val, strlen(val), SQLITE_STATIC);

  
  ret = sqlite3_step(stmt);
  if ( ret != SQLITE_DONE ) {
    fprintf(stderr, "SQL error in log message: %s :", sqlite3_errmsg(db));
    fprintf(stderr, "KEY: %s VALUE: %s\n", key, val);
    return 1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

