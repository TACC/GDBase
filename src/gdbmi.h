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

#ifndef __H_GDBMI
#define __H_GDBMI

#define GDB_STDERR 1
#define GDB_GDBMSG 2
#define GDB_BRKPNT 3
#define GDB_PRGEXT 4
#define GDB_STDOUT 5
#define GDB_SIGSEG 6
#define GDB_WPT 7
#define GDB_WPS 8
#define GDB_PROMPT 9
#define GDB_RUNNING 10
#define GDB_STEP 11
#define GDB_FINISH 12
#define GDB_SIGINT 13

#define GDB_DONE 1
#define GDB_ERROR 2

#define type_breakpoint 1
#define type_write_watchpoint 2
#define type_read_watchpoint 3
#define type_access_watchpoint 4

int gdb_setup(char *programname, char* gdb_location);
int gdb_set_arguments(char *args);
int gdb_start_run();
int gdb_continue();
int gdb_next_event();
int gdb_exit();
void gdb_interrupt();


char *gdb_lastoutput();

int gdb_tcl_lastoutput(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int gdb_tcl_setbreakpoint(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_setwatchpoint(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_evalExpr(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_continue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_getStackFrames(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_listLocals(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_stepNext(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_step(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_stepFinish(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_set_pid(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_call(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int gdb_tcl_set(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

char *gdb_dispatchBreakpoint();
char *gdb_dispatchWatchpointTrigger();
char *gdb_dispatchWatchpointScope();

#endif
