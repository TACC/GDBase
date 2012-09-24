#-------------------------------------------------------------------------------
# Custom MGF agent
# Doug James     (DJ)  - djames (at) tacc.utexas.edu
# Carlos Rosales (CRF) - carlos (at) tacc.utexas.edu
#
# This agent captures detailed information from several MPI function calls in 
# the MGF code, focusing in particular in the zero-byte synchronization calls
# made from function sendlists.C
#
# Notice that during the data collection one must be careful because gdb will 
# stop just before the fucntion call, so an extra gdb step must be taken before 
# the actual MPI function arguments can be evaluated. 
#-------------------------------------------------------------------------------

# This procedure sets a breakpoint at the very beginning of the program
# and defines and initializes the global call counter variables
proc user_setup {} {

    global countR
    global countIS
    global countIR
    global countIRS

    set countR   1
    set countIS  1
    set countIR  1
    set countIRS 1

    gdb_setBreakpoint "main" "mgf_setup"

}

# This procedure sets the breakpoints in the locations we are interested on,
# prints a message to the database to indicate the breakpoint has been set,
# and invokes the data collection routine "mgf_catch"
proc mgf_setup {} {

	gdb_setBreakpoint "sendlists.C:112" "mgf_catch \"irecv\""
	set output [ gdb_lastOutput]
	db_logMessage "agent.irecv" $output

	gdb_setBreakpoint "sendlists.C:117" "mgf_catch \"isend_byte\""
	set output [ gdb_lastOutput]
	db_logMessage "agent.isend_byte" $output

	gdb_setBreakpoint "sendlists.C:121" "mgf_catch \"recv_byte\""
	set output [ gdb_lastOutput]
	db_logMessage "agent.recv_byte" $output

	gdb_setBreakpoint "sendlists.C:129" "mgf_catch \"irsend\""
	set output [ gdb_lastOutput]
	db_logMessage "agent.irsend" $output

	gdb_continue
}

# Record gdb step in database
proc report_step { } {
	db_logMessage "agent.step" "Stepped into function"
}

# This procedure interacts with gdb to capture program execution information 
# and prints the results to a database file for every MPI rank.
proc mgf_catch { type } {
    global countR
    global countIS
    global countIR
    global countIRS

    set var ""
    set dest ""
    set tag ""
    set rank ""

    # Get the traceback to see where this call is coming from
    gdb_getStackFrames
    set stackOutput [ gdb_lastOutput ]

    # Find out which MPI task is making this call
    # The variable "I_am" is locally defined in the MGF code and represents
    # the MPI rank of the current task.
    set rank [ gdb_evalExpr "I_am" ]

    # Collect irecv call data
    if { $type == "irecv" } {

        # Define a variable with the call name and count for convenience
	set var "irecv.$countIR"

        # Write stackframe and local variable information to database file
        # db_logMessage "agent.$var" $localsOutput
        db_logMessage "agent.$var" $stackOutput

        # Step into the MPI_Recv call itself
        gdb_step "report_step"

        # Evaluate MPI_Recv arguments
        set size  [ gdb_evalExpr "count" ]
        set src   [ gdb_evalExpr "source" ]
        set tag   [ gdb_evalExpr "tag" ]

        # Write overall information to database file
        db_logMessage "agent.$var" "^stp=,rank=\"$rank\",source=\"$src\",tag=\"$tag\",size=\"$size\""

        # Increase call counter by one
	    set countR [expr $countIR+1 ]
	}

    # Collect recv_byte call data
    if { $type == "recv_byte" } {

        # Define a variable with the call name and count for convenience
	set var "recv.$countR"

        # Write stackframe and local variable information to database file
        # db_logMessage "agent.$var" $localsOutput
        db_logMessage "agent.$var" $stackOutput

        # Step into the MPI_Recv call itself
        gdb_step "report_step"

        # Evaluate MPI_Recv arguments
        set size  [ gdb_evalExpr "count" ]
        set src   [ gdb_evalExpr "source" ]
        set tag   [ gdb_evalExpr "tag" ]

        # Write overall information to database file
        db_logMessage "agent.$var" "^stp=,rank=\"$rank\",source=\"$src\",tag=\"$tag\",size=\"$size\""

        # Increase call counter by one
	    set countR [expr $countR+1 ]
	}

    # Collect irsend call data
    if { $type == "irsend" } {

        # Define a variable with the call name and count for convenience
	set var "irsend.$countIRS"

        # Write stackframe and local variable information to database file
        # db_logMessage "agent.$var" $localsOutput
        db_logMessage "agent.$var" $stackOutput

        # Step into the MPI_Recv call itself
        gdb_step "report_step"

        # Evaluate MPI_Recv arguments
        set size  [ gdb_evalExpr "count" ]
        set dest  [ gdb_evalExpr "dest" ]
        set tag   [ gdb_evalExpr "tag" ]

        # Write overall information to database file
        db_logMessage "agent.$var" "^stp=,rank=\"$rank\",dest=\"$dest\",tag=\"$tag\",size=\"$size\""

        # Increase call counter by one
        set countR [expr $countIRS+1 ]
	}

    # Collect isend_byte call data
    if { $type == "isend_byte" } {

        # Define a variable with the call name and count for convenience
	set var "isend_byte.$countIS"

        # Write stackframe and local variable information to database file
        # db_logMessage "agent.$var" $localsOutput
        db_logMessage "agent.$var" $stackOutput

        # Step into the MPI_Recv call itself
        gdb_step "report_step"

        # Evaluate MPI_Recv arguments
        set size  [ gdb_evalExpr "count" ]
        set dest  [ gdb_evalExpr "dest" ]
        set tag   [ gdb_evalExpr "tag" ]

        # Write overall information to database file
        db_logMessage "agent.$var" "^stp=,rank=\"$rank\",dest=\"$dest\",tag=\"$tag\",size=\"$size\""

        # Increase call counter by one
        set countIS [expr $countIS+1 ]
	}

	gdb_continue
}
