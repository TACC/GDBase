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
#include <termios.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>
#include <unistd.h>
#include "gdbmi.h"
#include "vector.h"

int gdb_p_stdout[2];
int gdb_p_stderr[2];
int gdb_p_stdin[2];
FILE *gdb_stdout;
FILE *gdb_stderr;
FILE *gdb_stdin;

char *line;
int linesize;

pid_t gdb_pid,child_pid;

Vector *breakpoints;
char *step_function;

typedef	struct s_bp 
{
  char *breakpoint;
  char *tclfunction;
  int bp_type;
} Breakpoint;

void gdb_waitforEvent(int eventtype);
void gdb_waitforprompt();

int gdb_setup(char *programname, char* gdb_location) 
{
  //Startup GDB in proper mode
  child_pid = 0;

  //setup 2 input pipes (stdout, stderr) and one output pipe (stdin) for GDB
  pipe(gdb_p_stdout );
  pipe(gdb_p_stderr );
  pipe(gdb_p_stdin );

  if ((gdb_pid = fork()) == 0)
    {
      //Child
      //setup my pipes by overwriting all standard pipes!
      //0 stdin 1 stdout 2 stderr
      dup2(gdb_p_stdin[0], 0);
      dup2(gdb_p_stdout[1], 1);
      dup2(gdb_p_stderr[1], 2);
      
      close(gdb_p_stdin[1]);
      close(gdb_p_stdout[0]);
      close(gdb_p_stderr[0]);
      
      //Exec GDB
      execl(gdb_location, "gdb", "--quiet", "--interpreter=mi", programname, NULL);
    }

  //setup our pipes
  gdb_stdout = fdopen (gdb_p_stdout[0], "r");
  gdb_stderr = fdopen (gdb_p_stderr[0], "r");
  gdb_stdin  = fdopen (gdb_p_stdin[1], "w");
  
  setvbuf(gdb_stdin,(char*)NULL,_IONBF,0);
  setvbuf(gdb_stdout,(char*)NULL,_IONBF,0);
  setvbuf(gdb_stderr,(char*)NULL,_IONBF,0);
  
  close(gdb_p_stdin[0]);
  close(gdb_p_stdout[1]);
  close(gdb_p_stderr[1]);
  
  line = NULL;
  linesize = 0;

  //Setup Breakpoints Vector
  breakpoints = vector_new();
  
  return 0;
}

void gdb_interrupt() 
{
  //printf("Killing %d\n", child_pid);
  kill(child_pid, SIGINT);

  gdb_waitforEvent(GDB_SIGINT);
}

void gdb_call(char* func, char* buffer) 
{
  char *location;
  int i;
  i = 0;


  fprintf(gdb_stdin, "-interpreter-exec console \"call %s\"\n", func);
  //printf("INPUT: -interpreter-exec console \"call %s\"\n", func);
  do
    {
      //printf("CALL nextevent\n");
      gdb_next_event();
    }
  while (NULL== strstr(gdb_lastoutput(), " = "));
  
  location = strstr(gdb_lastoutput(), " = ");
  location += strlen(" = ");

  while (*location != '\"')
    buffer[i++] = *location++;

  gdb_waitforEvent(GDB_PROMPT);

}

void gdb_set_pid()
{
  int i;
  char *location;
  char buffer[256];
  
  memset(buffer, '\0', sizeof(char)*256);
  i = 0;

  gdb_call("getpid()", buffer);

  i = atoi(buffer);

  child_pid = i;
}

void gdb_set(char *var, char *value)
{
  fprintf(gdb_stdin, "-gdb-set %s = %s \n", var, value);
  //printf("INPUT: -gdb-set %s = %s \n", var, value);
  gdb_getresponse();
}

int gdb_set_arguments(char *args) 
{
  
  //Start application with its arguments "-exec-arguments args "
  fprintf(gdb_stdin, "-exec-arguments %s\n", args);
  //printf("INPUT: -exec-arguments %s\n", args);
  return 0;
}

int gdb_start_run() 
{
  fprintf(gdb_stdin, "-exec-run\n");
  //printf("INPUT: -exec-run\n");
  return 0;
}

int gdb_continue()
{
  fprintf(gdb_stdin, "-exec-continue\n");
  //printf("INPUT: -exec-continue\n");
  return 0;
}

int gdb_tcl_continue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  
  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, gdb_continue());

  return TCL_OK;
}

int gdb_getresponse()
{
  int event, exitflag, count;
  exitflag = 0;
  count = 0;
  
  while (!exitflag)
    {
      //printf("GETRESPONSE nextevent\n");
      event = gdb_next_event();
      if (strstr(gdb_lastoutput(), "^done") == gdb_lastoutput())
        {
          //printf("GETRESPONSE ^done\n");
          exitflag = GDB_DONE;
        }
      else if (strstr(gdb_lastoutput(), "^error") == gdb_lastoutput())
        {
          //printf("GETRESPONSE ^error\n");
          exitflag = GDB_ERROR;
        }
      // else if (strstr(gdb_lastoutput(), "*stopped") == gdb_lastoutput()) {
      // 	printf("GETRESPONSE *stopped\n");
      // 	exitflag = GDB_DONE;
      // }
    }

  return exitflag;
}

void gdb_waitforprompt()
{
  int event, exitflag;
  exitflag = 0;
  
  while (!exitflag)
    {
      //printf("WAITFORPROMPT nextevent\n");
      event = gdb_next_event();
      if (strstr(gdb_lastoutput(), "(gdb)") == gdb_lastoutput())
        exitflag = 1;
    }
}

void gdb_waitforEvent(int eventtype)
{
  int event;

  while (1)
    {
      //printf("WAITFOREVENT nextevent\n");
      event = gdb_next_event();
      //printf("WAITING %d got %d\n", eventtype, event);
      fflush(stdout);
      if (event == eventtype)
        break;

      if (event == GDB_PRGEXT || event == GDB_SIGSEG)
        {
          //printf("WAITFOREVENT GOT EXIT\n");
          fflush(stdout);
          break;
        }
    }
}

int gdb_next_event() 
{
  char *temp;

  /*
    #define GDB_STDERR  1
    #define GDB_GDBMSG  2
    #define GDB_BRKPNT  3
    #define GDB_PRGEXT  4
    #define GDB_STDOUT  5
    #define GDB_SIGSEG  6
    #define GDB_WPT     7
    #define GDB_WPS     8
    #define GDB_PROMPT  9
    #define GDB_RUNNING 10
    #define GDB_STEP    11
    #define GDB_FINISH  12
    #define GDB_SIGINT  13
  */

  fd_set set;

  FD_ZERO( &set );
  FD_SET(gdb_p_stderr[0], &set);
  FD_SET(gdb_p_stdout[0], &set);
  
  int ret;
  ret = select( FD_SETSIZE, &set, NULL, NULL, NULL );

  if (ret < 0) return -1;

  if (FD_ISSET(gdb_p_stderr[0], &set))
    {
      getline(&line, &linesize, gdb_stderr);
      //printf("   stderr: %s", line);
      fflush(stdout);
      return GDB_STDERR;
    }
  if (FD_ISSET(gdb_p_stdout[0], &set))
    {
      getline(&line, &linesize, gdb_stdout);
      //printf("   stdout: %s", line);
      fflush(stdout);
      
      // Parse & find type of message

      //MOST OF THIS CODE SHOULD BE MERGED WITH OPD LOOP!
      if (line[0] == '~')
        return GDB_GDBMSG;
      // *stopped,reason="breakpoint-hit",bkptno="1",thread-id="1",frame={addr="0x000000000040079f",func="mksegfault2",args=[{name="value",value="16"}],file="mpi_target.c",line="13"}
      temp = strstr(line, "*stopped");

      if (temp)
        {
          if (strstr(line, "breakpoint-hit")) return GDB_BRKPNT;
          if (strstr(line, "watchpoint-trigger")) return GDB_WPT;
          if (strstr(line, "watchpoint-scope")) return GDB_WPS;
          if (strstr(line, "SIGSEGV")) return GDB_SIGSEG;
          if (strstr(line, "SIGBUS")) return GDB_SIGSEG;
          if (strstr(line, "end-stepping-range")) return GDB_STEP;
          if (strstr(line, "function-finished")) return GDB_FINISH;
          if (strstr(line, "SIGINT")) return GDB_SIGINT;
          //			if (strstr(line, "exited-normally")) {
          //				printf("NEXTEVENT GOT EXIT\n");
          //				fflush(stdout);
          //				return GDB_PRGEXT;
          //			}

          //printf("NEXTEVENT EXIT\n");
          fflush(stdout);
          
          return GDB_PRGEXT;
        }

      if (strstr(line, "^running")) return GDB_RUNNING;

      if (strstr(line, "(gdb)")) return GDB_PROMPT;

      temp = strstr(line, "^error");
      if (temp)
        {
          if (strstr(line, "The program is not being run.")) return GDB_PRGEXT;
        }
      return GDB_STDOUT;
    }
  //printf("-1\n");

  return -1;
}

char *gdb_lastoutput()
{
  return line;
}

int gdb_tcl_lastoutput(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  objPtr = Tcl_GetObjResult(interp);
  Tcl_SetStringObj(objPtr, line, strlen(line));
  
  return TCL_OK;
}

char *gdb_dispatchBreakpoint()
{
  char buffer[256];
  int i;
  Breakpoint *b;
  char *location;
  
  i = 0;

  memset(buffer, '\0', sizeof(buffer));

  location = strstr(gdb_lastoutput(), "bkptno=");
  location += strlen("bkptno=") + 1;
  
  while (*location != '\"')
    buffer[i++] = *location++;

  i = atoi(buffer) - 1;

  b = (Breakpoint*) vector_getItem(breakpoints,i);

  return b->tclfunction;
}

int gdb_tcl_setbreakpoint(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *bp;
  char *func;
  int strlenbp,strlenmsg;
  Tcl_Obj *objPtr;
  Breakpoint *b;
  
  if (objc!=3)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  //Get string for breakpoint
  bp = Tcl_GetStringFromObj(objv[1], &strlenbp);
  //Save whom to call when we break
  func = Tcl_GetStringFromObj(objv[2], &strlenmsg);
  
  //Create data struture element, insert break point into it
  b = (Breakpoint*) malloc(sizeof(Breakpoint));
  b->breakpoint = (char*) malloc((strlen(bp)+1)*sizeof(char));
  b->tclfunction = (char*) malloc((strlen(func)+1)*sizeof(char));

  strcpy(b->breakpoint,bp);
  strcpy(b->tclfunction,func);

  b->bp_type = type_breakpoint;


  vector_addItem(breakpoints, b);

  //Set breakpoint /w GDB
  fprintf(gdb_stdin, "-break-insert %s\n", bp);
  //printf("INPUT: -break-insert %s\n", bp);

  //Find result
  gdb_getresponse();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

char *gdb_dispatchWatchpointTrigger()
{
  char buffer[256];
  int i;
  Breakpoint *b;
  char *location;

  i = 0;

  memset(buffer, '\0', sizeof(buffer));

  location = strstr(gdb_lastoutput(), "wpt={number=");
  location += strlen("wpt={number=") + 1;
  while (*location != '\"')
    buffer[i++] = *location++;

  i = atoi(buffer) - 1;

  b = (Breakpoint*) vector_getItem(breakpoints,i);

  return b->tclfunction;
}

char *gdb_dispatchWatchpointScope()
{
  char buffer[256];
  int i;
  Breakpoint *b;
  char *location;

  i = 0;
  
  memset(buffer, '\0', sizeof(buffer));
  
  location = strstr(gdb_lastoutput(), "wpnum=");
  location += strlen("wpnum=") + 1;
  
  while (*location != '\"')
    buffer[i++] = *location++;

  i = atoi(buffer) - 1;

  b = (Breakpoint*) vector_getItem(breakpoints,i);

  return b->tclfunction;
}

int gdb_tcl_setwatchpoint(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *watchpoint_type;
  char *watchpoint_var;
  char *watchpoint_function;
  int strlen_type, strlen_var, strlen_function;
  Tcl_Obj *objPtr;
  Breakpoint *b;
  
  if (objc != 4)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  //Get string for watchpoint type
  watchpoint_type = Tcl_GetStringFromObj(objv[1], &strlen_type);
  //Get the watched variable
  watchpoint_var = Tcl_GetStringFromObj(objv[2], &strlen_var);
  //Get the tcl function
  watchpoint_function = Tcl_GetStringFromObj(objv[3], &strlen_function);
  
  //Create data struture element, insert break point into it
  b = (Breakpoint*) malloc(sizeof(Breakpoint));

  b->breakpoint = (char*)malloc(sizeof(char)*strlen(watchpoint_var));
  b->tclfunction = (char*)malloc(sizeof(char)*strlen(watchpoint_function));

  strcpy(b->breakpoint  , watchpoint_var);
  strcpy(b->tclfunction , watchpoint_function);
  
  if (watchpoint_type[0] == 'a')
    {
      b->bp_type= type_access_watchpoint;
      fprintf(gdb_stdin, "-break-watch -a %s\n", watchpoint_var);
      //printf("INPUT: -break-watch -a %s\n", watchpoint_var);
    }
  else if (watchpoint_type[0] == 'r')
    {
      b->bp_type = type_read_watchpoint;
      fprintf(gdb_stdin, "-break-watch -r %s\n", watchpoint_var);
      //printf("INPUT: -break-watch -r %s\n", watchpoint_var);
    }
  else
    {
      b->bp_type = type_write_watchpoint;
      fprintf(gdb_stdin, "-break-watch %s\n", watchpoint_var);
      //printf("INPUT: -break-watch %s\n", watchpoint_var);
    }
  fflush(stdout);

  //Find result
  if (gdb_getresponse() == GDB_DONE)
    vector_addItem(breakpoints, b);

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);
  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_evalExpr(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *expr;
  int strlenbp;
  Tcl_Obj *objPtr;
  char *location, *temp;
  
  if (objc!=2)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  expr = Tcl_GetStringFromObj(objv[1], &strlenbp);

  fprintf(gdb_stdin, "-data-evaluate-expression %s\n", expr);
  //printf("INPUT: -data-evaluate-expression %s\n", expr);

  //Find result
  gdb_getresponse();

  //Obtain objPtr
  objPtr = Tcl_GetObjResult(interp);


  //Return what comes after value= to next " as result
  location = strstr(gdb_lastoutput(), "value=");


  if (location == NULL)
    Tcl_SetStringObj(objPtr, "Error", strlen("Error"));
  else
    {
      location += strlen("value=") + 1;
      temp = location; //move to first 'value character'
      
      //find last " character
      while (*location != '\"')
        location++;
      
      Tcl_SetStringObj(objPtr, temp, (location-temp)/sizeof(char));
    }
  
  return TCL_OK;
}

int gdb_tcl_listLocals(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  
  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  fprintf(gdb_stdin, "-stack-list-locals --simple-values\n");
  //printf("INPUT: -stack-list-locals --simple-values\n");

  //Find result
  gdb_getresponse();

  objPtr = Tcl_GetObjResult(interp);

  //	Tcl_SetStringObj(objPtr, gdb_lastoutput(),strlen(gdb_lastoutput()));
  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_getStackFrames(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	Tcl_Obj *objPtr;

	if (objc!=1) {
		Tcl_WrongNumArgs(interp, 1, objv, "value");
		return TCL_ERROR;
	}

	// fprintf(gdb_stdin, "-stack-list-frames --all-values\n");
	fprintf(gdb_stdin, "-stack-list-frames\n");
	//printf("INPUT: -stack-list-frames\n");

	//Find result
	gdb_getresponse();

	//Return breakpoint number as result
	objPtr = Tcl_GetObjResult(interp);

	Tcl_SetIntObj(objPtr, 1);

	return TCL_OK;
}

int gdb_tcl_getStackArgs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  
  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  fprintf(gdb_stdin, "-stack-list-arguments 1\n");
  //printf("INPUT: -stack-list-arguments 1\n");

  //Find result
  gdb_getresponse();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_stepNext(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;

  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  fprintf(gdb_stdin,"-exec-next\n");
  //printf("INPUT: -exec-next\n");

  //Find result
  //gdb_getresponse();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_step(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char* message;
  int strlenmsg;
  Tcl_Obj *objPtr;
  
  if (objc!=2)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  message = Tcl_GetStringFromObj(objv[1], &strlenmsg);
  step_function = (char*) malloc(strlenmsg * sizeof(char));
  strcpy(step_function, message);
  
  fprintf(gdb_stdin,"-exec-step\n");
  //printf("INPUT: -exec-step\n");

  //Find result
  //gdb_getresponse();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_stepFinish(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  
  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  fprintf(gdb_stdin,"-exec-finish\n");
  //printf("INPUT: -exec-finish\n");

  //Find result
  //gdb_getresponse();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

char *gdb_dispatchStepFunction()
{
	return step_function;
}

int gdb_tcl_set_pid(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  
  if (objc!=1)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  gdb_set_pid();

  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetIntObj(objPtr, 1);

  return TCL_OK;
}

int gdb_tcl_set(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  char *var;
  char *val;
  int size;
  
  if (objc!=3)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  var = Tcl_GetStringFromObj(objv[1], &size);
  val =  Tcl_GetStringFromObj(objv[2], &size);

  gdb_set(var,val);



  //Return breakpoint number as result
  objPtr = Tcl_GetObjResult(interp);
  
  Tcl_SetIntObj(objPtr, 1);
  
  return TCL_OK;
}

int gdb_tcl_call(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objPtr;
  int strlenf;
  char *func;
  char buffer[256];
  
  memset(buffer, '\0', sizeof(buffer));

  if (objc!=2)
    {
      Tcl_WrongNumArgs(interp, 1, objv, "value");
      return TCL_ERROR;
    }

  func = Tcl_GetStringFromObj(objv[1], &strlenf);

  gdb_call(func, buffer);

  objPtr = Tcl_GetObjResult(interp);

  Tcl_SetStringObj(objPtr, buffer, strlen(buffer));

	return TCL_OK;
}

int gdb_exit()
{
  fprintf(gdb_stdin,"-gdb-exit\n");
  //printf("INPUT: -gdb-exit\n");
  return 0;
}
