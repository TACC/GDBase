# This file is part of GDBase.
#
# GDBase is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GDBase is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDBase.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright 2009 Anthony DiGirolamo, Dan Stanzione, Karl Lindekugel


proc user_setup {} {
	gdb_setBreakpoint "main" "deadlock_setup"

}

proc deadlock_setup {} {

	# Set breakpoint on Send, Recv,
	# Probably use one function, with name as argument 
	gdb_setPID

	#gdb_setBreakpoint "MPI_Allgatherv" "allgatherv_catch"
	gdb_setBreakpoint "MPI_Send" "deadlock_catch \"Send\""
	set output [ gdb_lastOutput]
	db_logMessage "deadlock.setup.Send" $output

	gdb_setBreakpoint "MPI_Recv" "deadlock_catch \"Recv\""
        set output [ gdb_lastOutput]
        db_logMessage "deadlock.setup.Recv" $output

	gdb_setBreakpoint "MPI_Isend" "deadlock_catch \"Isend\""
	set output [ gdb_lastOutput]
	db_logMessage "deadlock.setup.Isend" $output

	gdb_setBreakpoint "MPI_Irecv" "deadlock_catch \"Irecv\""
        set output [ gdb_lastOutput]
	db_logMessage "deadlock.setup.Irecv" $output


	gdb_continue
}

proc deadlock_catch { type } {

	gdb_stepNext


	set count  [ gdb_evalExpr "count" ]

	set var ""

	# Log other stuff here!like src and dest
	if { $type == "Send" || $type == "Isend" } {
		set var "dest"
	}

	if { $type == "Recv"  || $type == "Irecv" } {
		set var "source"
        }

	set remote [gdb_evalExpr $var]

	gdb_call "MPI_Type_size(type,&$var)"

	set size [gdb_evalExpr $var]

	gdb_set $var $remote

	set tag [gdb_evalExpr "tag"]

	db_logMessage "deadlock.$type.values" "^stp=,remote=\"$remote\",count=\"$count\",size=\"$size\",tag=\"$tag\""

	gdb_continue
}

