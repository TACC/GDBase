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
 * Copyright 2012 Robert McLay, Carlos Rosales, Doug James
 *
 * ********************************************************************
 * 
 *  30 Apr 2012 -- Fixed indentation on a set of improperly matched 
 *                 braces << while (!exitflag) >>. (DJ)
 *
 **********************************************************************

 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <tcl.h>
#include <unistd.h>
#include <getopt.h>

#include "dblog.h"
#include "gdbmi.h"
#include "parser.h"
#include "findexec.h"

#define PROGRAM_NAME "gdbase"

const char * mpi_job_id [] =
{
  "PBS_JOBID",
  "JOB_ID",
  0
};

const char * mpi_rank_names [] =
{
  "OMPI_COMM_WORLD_RANK",
  "OMPI_MCA_orte_ess_vpid",
  "OMPI_MCA_ns_nds_vpid",
  "PMI_RANK",
  "PMI_ID",
  "MPIRUN_RANK",
  0
};
  
const char * mpi_nproc_names [] =
{
  "OMPI_COMM_WORLD_SIZE",
  "OMPI_MCA_orte_ess_num_procs",
  "OMPI_MCA_ns_nds_num_procs",
  "PMI_SIZE",
  "MPIRUN_NPROCS",
  0
};

//Global
volatile int exitflag;
int rank, size;
Tcl_Interp* interp;

void opd_exit(int normal);

int exists(const char *filename) {
  return !access(filename, F_OK);
}

int opd_tcl_setExit(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
  Tcl_Obj *objPtr;
  objPtr = Tcl_GetObjResult(interp);
  exitflag = 1;
  Tcl_SetIntObj(objPtr, 0);
  return TCL_OK;
}

int opd_tcl_getRank(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
  Tcl_Obj *objPtr;
  objPtr = Tcl_GetObjResult(interp);
  Tcl_SetIntObj(objPtr, rank);
  return TCL_OK;
}

int opd_tcl_getSize(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
  Tcl_Obj *objPtr;
  objPtr = Tcl_GetObjResult(interp);
  Tcl_SetIntObj(objPtr, size);
  return TCL_OK;
}

void printhelp() {
  printf("Usage: gdbase [OPTIONS]... -e EXEC -a ARGS\n");
  printf("\n");
  printf("Options: \n");
  printf("  -e  --exec        Target application\n");
  printf("  -a  --args        Target application arguments\n");
  printf("  Examples:\n");
  printf("    gdbase -e MyApp\n");
  printf("    gdbase -e MyApp -a \"arg1 arg2\"\n");
  printf("\n");
  printf("  -h, --help        Display this message\n");
  printf("  -c, --config      Use a custom config file\n");
  printf("\n");
  printf("  -d, --debug-spec  Use a debugging specification file \n");
  printf("  -s, --script      Use a custom debugging script\n");
  printf("  -t, --timeout	    Set a time out for message collection in seconds\n");
  printf("\n");
  printf("  -p, --prefix      Specify the GDBase prefix\n");
  printf("                     By default, read from GDBASE_PREFIX\n");
  printf("  -g, --gdb_binary  Specify which gdb to use\n");
  printf("                     Default: /usr/bin/gdb or read from GDB_BINARY\n");
  printf("  -T, --temp        Temporary file storage location\n");
  printf("                     Default: ./\n");
  printf("\n");
  printf("  -j, --jobid       Use a custom string as the job id for database logging\n");
  printf("  -J  --jobid_var   Environment variable for job id\n");
  printf("                     Default: PBS_JOBID:JOB_ID\n");
  printf("  -r, --rank_var    Environment variable for the MPI process rank/id\n");
  printf("                     Default for MVAPICH: MPIRUN_RANK\n");
  printf("                     Default for OpenMPI: OMPI_MCA_ns_nds_vpid\n");
  printf("  -n, --nprocs_var  Environment variable for MPI num procs\n");
  printf("                     Default for MVAPICH: MPIRUN_NPROCS\n");
  printf("                     Default for OpenMPI: OMPI_MCA_ns_nds_num_procs\n");
}

void catch_alarm (int sig) 
{
  //Log Message
  gdb_interrupt();
  Tcl_Eval(interp, "opd_dispatchInterrupt");
  opd_exit(1);
}

int main(int argc, char* argv[]) 
{
  const char **arrayp;
  char *config_file = NULL;
  int using_mpi = 1; // assume we use mpi

  // Environment variable names
  char *jobid = NULL;
  char *jobid_var = NULL;
  char *rank_var = NULL;
  char *nprocs_var = NULL;

  // Timeout + target app
  char *timeout_string = NULL;
  int timeout = 0;
  char *target = NULL;
  char *arguments = NULL;
  char target_basename[512];

  // Script files
  char *spec_file = NULL;
  char *script_file = NULL;

  // Directories
  char *prefix = NULL;
  char *opd_dir = NULL;
  char *gdb_location = NULL;
  char *temp_dir = NULL;
  char working_dir[4096];

  char *temp;
  char strbuf[4096];
  int i = 0;
  int c = 0; // for getopt
  int target_index = 0;
  rank = 0;
  size = 0;

  signal(SIGALRM, catch_alarm);

  getcwd(working_dir, 4096);

  // Parse the Command Line

  while (c != -1) {
    static struct option long_options[] = {
      {"exec",		required_argument, 0, 'e'},
      {"args",		required_argument, 0, 'a'},
      {"help",		no_argument,       0, 'h'},
      {"config",	required_argument, 0, 'c'},
      {"debug-spec",	required_argument, 0, 'd'},
      {"script",	required_argument, 0, 's'},
      {"timeout",	required_argument, 0, 't'},
      {"prefix",	required_argument, 0, 'p'},
      {"gdb_binary",	required_argument, 0, 'g'},
      {"temp",		required_argument, 0, 'T'},
      {"jobid",		required_argument, 0, 'j'},
      {"jobid_var",	required_argument, 0, 'J'},
      {"rank_var",	required_argument, 0, 'r'},
      {"nprocs_var",	required_argument, 0, 'n'},
      {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long (argc, argv, "e:a:hc:d:s:t:p:g:T:j:J:r:n:", long_options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 'e':
      target = optarg;
      break;
    case 'a':
      arguments = optarg;
      break;
    case 'h':
      printhelp();
      return EXIT_SUCCESS;
      break;
    case 'c':
      config_file = optarg;
      break;
    case 'd':
      spec_file = optarg;
      break;
    case 's':
      script_file = optarg;
      break;
    case 't':
      timeout_string = optarg;
      break;
    case 'p':
      opd_dir = optarg;
      break;
    case 'g':
      gdb_location = optarg;
      break;
    case 'T':
      temp_dir = optarg;
      break;
    case 'j':
      jobid = optarg;
      break;
    case 'J':
      jobid_var = optarg;
      break;
    case 'r':
      rank_var = optarg;
      break;
    case 'n':
      nprocs_var = optarg;
      break;
    default:
      abort();
    }
  }

  // Did we include a target app?
  if (!target) 
    {
      fprintf(stderr, "%s: Missing target application. Try -h for help.\n",
              PROGRAM_NAME);
      exit(EXIT_FAILURE);
    }

  // Does the target app exist?
  if (!exists(target)) 
    {
      fprintf(stderr, "%s: Cannot find target application: %s\n",
              PROGRAM_NAME, target);
      exit(EXIT_FAILURE);
    }

  // Get the file name of the target app
  temp = NULL;
  temp = strchr(target, '/');
  if (temp) 
    {
      temp = basename(target);
      strcpy(target_basename, temp);
    } 
  else
    strcpy(target_basename, target);

  // Get environment variables

  // MPI Vars
  if (rank_var != NULL)
    {
      if (getenv(rank_var)) 
        {
          temp = getenv(rank_var);
          rank = atoi(temp);
        }
    }
  if (nprocs_var != NULL)
    {
      if (getenv(nprocs_var)) 
        {
          temp = getenv(nprocs_var);
          size = atoi(temp);
        }
    }

  
  /* Search for env var that hold the mpi rank*/

  for (arrayp = mpi_rank_names; *arrayp; ++arrayp)
    {
      if (getenv(*arrayp))
        {
          rank = atoi(getenv(*arrayp));
          break;
        }
    }
  
  /* Search for env var that hold the mpi size*/

  for (arrayp = mpi_nproc_names; *arrayp; ++arrayp)
    {
      if (getenv(*arrayp))
        {
          size = atoi(getenv(*arrayp));
          break;
        }
    }
  
  if (!rank && !size) 
    {
      using_mpi = 0;
      rank = 0;
      size = 1;
      printf("%s: MPI not found, running without.\n",
             PROGRAM_NAME);
    }

  // If jobid wasn't manually set
  if (jobid == NULL) 
    {
      // check for a custom env var
      if (jobid_var) 
        {
          if (getenv(jobid_var))
            jobid = getenv(jobid_var);
        } 
      else // check for PBS
	{
          jobid = "UNKNOWNID";
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
  // Prefix
  if (opd_dir == NULL)
    {
      int result;
      int n = 10;
      opd_dir = (char *) malloc(n);
      result = find_gdbase_prefix("opd", &n, &opd_dir);

      if (result != 1)
        {
          free(opd_dir);
          opd_dir = NULL;
        }
    }

  if (getenv("GDBASE_PREFIX"))
    opd_dir = getenv("GDBASE_PREFIX");
  sprintf(strbuf, "%s/bin/opd", opd_dir);
  if (!exists(strbuf)) 
    {
      if (rank==0)
        fprintf(stderr, "%s: Cannot find GDBase in '%s' please specify a prefix.\n",
                PROGRAM_NAME, opd_dir);
      exit(EXIT_FAILURE);
    }

  // GDB
  if (gdb_location == NULL)
    {
      int result;
      int n = 10;
      gdb_location = (char *) malloc(n);
      result = find_exec_in_path("gdb", &n, &gdb_location);
      if (result != 1)
        {
          free(gdb_location);
          gdb_location = NULL;
        }
    }
  if (getenv("GDB_BINARY"))
    gdb_location = getenv("GDB_BINARY");
  if (!exists(gdb_location)) 
    {
      if (rank==0)
        fprintf(stderr, "%s: Cannot find gdb executable.\n",
                PROGRAM_NAME);
      exit(EXIT_FAILURE);
    }

  if (timeout_string != NULL) 
    {
      timeout = atoi(timeout_string);
      if (timeout < 1 ) 
        {
          if (rank==0)
            fprintf(stderr, "%s: Invalid timeout value: %s\n",
                    PROGRAM_NAME, timeout_string);
          exit(EXIT_FAILURE);
        }
    }

  // TODO
  // Read Config File and update options that are NULL
  // (i.e. not already read from env or the command line)


  // Print Summary for jobscript output
  if (rank == 0) 
    {
      printf("== GDBase v0.1\n");
      printf("== argv[0]:             %s\n", argv[0]);
      printf("== Prefix:              %s\n", opd_dir);
      printf("== Using gdb:           %s\n", gdb_location);
      if (config_file)
        printf("== Using config file:   %s\n", config_file);
      printf("==\n");
      printf("== Working Directory:   %s\n", working_dir);
      printf("== Target Application:  %s\n", target_basename);
      if (arguments)
        printf("== Target Arguments:    %s\n", arguments);
      printf("== Job ID:              %s\n", jobid);
      printf("== Number of Processes: %d\n", size);
      if (spec_file)
        printf("== Spec File:           %s\n", spec_file);
      if (script_file)
        printf("== Script File:         %s\n", script_file);
      if (timeout)
        printf("== Timeout (s):         %d\n", timeout);
      if (temp_dir)
        printf("== Temp File Directory: %s\n", temp_dir);
    }

  // Name and open the new sqlite file
  sprintf(strbuf, "%s-%s.%d", "gdblog", target_basename, rank);

  if (temp_dir != NULL)
    sprintf(strbuf, "%s/%s-%s.%d", temp_dir, "gdblog", target_basename, rank);

  if (exists(strbuf))
    remove(strbuf);
  if (db_open(strbuf)) 
    {
      fprintf(stderr, "%s: Temporary log file could not be opened: %s\n",
              PROGRAM_NAME, strbuf);
      exit(EXIT_FAILURE);
    }

  if (arguments)
    sprintf(strbuf, "Executing: %s %s\n", target, arguments);
  else
    sprintf(strbuf, "Executing: %s\n", target);
  db_logMessage("opd.message", strbuf);

  // Startup Interpreter
  interp = Tcl_CreateInterp();

  Tcl_Channel errors;

  // Setup Interpreter Commands
  // Move this to each .h file? db_TclSetup(interp*)?
  Tcl_CreateObjCommand(interp, "db_logMessage",      db_tcl_logMessage,      (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_lastOutput",     gdb_tcl_lastoutput,     (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_setBreakpoint",  gdb_tcl_setbreakpoint,  (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_setWatchpoint",  gdb_tcl_setwatchpoint,  (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_getStackFrames", gdb_tcl_getStackFrames, (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_getStackArgs",   gdb_tcl_getStackFrames, (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_evalExpr",       gdb_tcl_evalExpr,       (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_listLocals",     gdb_tcl_listLocals,     (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_continue",       gdb_tcl_continue,       (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "opd_setExit",        opd_tcl_setExit,        (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_stepNext",       gdb_tcl_stepNext,       (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_step",           gdb_tcl_step,           (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_stepFinish",     gdb_tcl_stepFinish,     (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_setPID",         gdb_tcl_set_pid,        (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_call",           gdb_tcl_call,           (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "gdb_set",            gdb_tcl_set,            (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateObjCommand(interp, "opd_getRank",        opd_tcl_getRank,        (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateObjCommand(interp, "opd_getSize",        opd_tcl_getSize,        (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);

  //Load default coding
  sprintf(strbuf, "%s/share/gdbase/opd.tcl", opd_dir);
  if (Tcl_EvalFile(interp, strbuf) != TCL_OK) 
    {
      errors = Tcl_GetStdChannel(TCL_STDERR);
      if (errors) 
        {
          fprintf(stderr, "%s: opd.tcl errors:\n", PROGRAM_NAME);
          Tcl_WriteObj(errors, Tcl_GetObjResult(interp));
          Tcl_WriteChars(errors, "\n", 1);
        }
      opd_exit(1);
      return 1;
    }

  //Startup GDB in proper mode
  gdb_setup(target, gdb_location);
  if (arguments)
    gdb_set_arguments(arguments);

  if (script_file) 
    {
      // db_logMessage("opd.message", strcat("Processing user script: ",strbuf));
      if (Tcl_EvalFile(interp, script_file) != TCL_OK) 
        {
          errors = Tcl_GetStdChannel(TCL_STDERR);
          if (errors) 
            {
              printf("%s errors:\n", script_file);
              Tcl_WriteObj(errors, Tcl_GetObjResult(interp));
              Tcl_WriteChars(errors, "\n", 1);
            }
          opd_exit(1);
          return 1;
        }
    }

  Tcl_Eval(interp, "opd_setup");

  if (spec_file) 
    {
      if (exists(spec_file)) 
        {
          sprintf(strbuf, "opd_processscript \"%s\"", spec_file);
          Tcl_Eval(interp, strbuf);
        }
      else 
        {
          printf("couldn't read file \"%s\": no such file or directory\n", spec_file);
          opd_exit(1);
          return 1;
        }
    }

  //Program block to hold temporary stack variable
  {
    time_t curtime = time (NULL);
    sprintf(strbuf, "Starting: %s", asctime( localtime(&curtime) ));
  }

  db_logMessage("opd.message", strbuf);

  gdb_start_run();

  int event;

  exitflag = 0;

  do 
    {
      alarm(timeout);
      //printf("OPDDO next_event\n");
      fflush(stdout);
      
      event = gdb_next_event();
      
      //printf("OPD: Event %d\n", event);
      fflush(stdout);
      
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

      switch (event) 
        {
        case GDB_STDERR:
          Tcl_Eval(interp, "opd_dispatchStdErr");
          break;
        case GDB_GDBMSG:
          Tcl_Eval(interp, "opd_dispatchGDB");
          break;
        case GDB_BRKPNT:
          Tcl_Eval(interp, gdb_dispatchBreakpoint());
          break;
        case GDB_WPT:
          Tcl_Eval(interp, gdb_dispatchWatchpointTrigger());
          break;
        case GDB_WPS:
          Tcl_Eval(interp, gdb_dispatchWatchpointScope());
          break;
        case GDB_STEP:
          //printf("OPD DO got GDB_STEP\n");
          Tcl_Eval(interp, (char*)gdb_dispatchStepFunction());
          break;
        case GDB_SIGSEG:
          Tcl_Eval(interp, "opd_dispatchSigSeg");
          break;
        case GDB_STDOUT:
          Tcl_Eval(interp, "opd_dispatchStdOut");
          break;
        case GDB_PRGEXT:
          //printf("OPD GOT EXIT\n");
          fflush(stdout);
          Tcl_Eval(interp, "opd_dispatchPrgExt");
          exitflag = 2;
          break;
          
          //DO NOTHING
        case GDB_RUNNING:
        case GDB_PROMPT:
        case 0:
          break;
        default:
          Tcl_Eval(interp, "opd_dispatchOther");
          break;
        }
      
  } while (!exitflag);

  if (exitflag==1 && using_mpi)
    opd_exit(0);

  opd_exit(1);

  return 0;
}

void opd_exit(int normal) 
{
  //Shutdown database
  db_close();
  //exit program
  gdb_exit();
  //Only do this if we crash, otherwise exit normally
  if (!normal)
    abort();
}
