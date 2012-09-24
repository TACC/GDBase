# Single step through an application
proc user_setup {} {
	gdb_setBreakpoint "main" "step_setup"
	db_logMessage "opd.breakOnMain" [gdb_lastOutput]
}

proc step_setup {} {
	gdb_step "step_function"
}

proc step_function {} {
	gdb_getStackFrames
	db_logMessage "opd.step.stack" [gdb_lastOutput]

	gdb_listLocals
	db_logMessage "opd.step.locals" [gdb_lastOutput]

	gdb_step "step_function"
}
