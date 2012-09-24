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
# Copyright 2012 Robert McLay, Carlos Rosales, Doug James
#
# **********************************************************************
#
# 24 Apr 2012 -- changed name from allgather.tcl to allgatherv.tcl (DJ)
#
# **********************************************************************


proc user_setup {} {
	global count 

	set count 0

	gdb_setBreakpoint "main" "allgatherv_setup"
}

proc allgatherv_setup {} {
	gdb_setBreakpoint "MPI_Allgatherv" "allgatherv_catch"
	set output [ gdb_lastOutput]
	db_logMessage "allgatherv.setup" $output
	gdb_continue
}

proc allgatherv_catch {} {
	global count

	gdb_getStackFrames

	set output [ gdb_lastOutput ]
	db_logMessage "allgatherv.catch.$count" $output

	gdb_stepNext

	set size [ opd_getSize ]

	set output [ gdb_evalExpr "sendcount" ]
	db_logMessage "allgatherv.$count.sendcount" $output

	for {set i 0} {$i<$size} {incr i} {
		db_logMessage "allgatherv.$count.recvcount.$i" [ gdb_evalExpr "recvcounts\[$i\]" ]
		db_logMessage "allgatherv.$count.displs.$i" [ gdb_evalExpr "displs\[$i\]" ]
	}

	set count [expr $count+1 ]

	gdb_continue
}

