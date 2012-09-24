#!/usr/bin/tclsh
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


proc opd_dispatchStdErr {} {
	# Check for an overriding user dispatch function

	#otherwise, simply log to database

	set output  [gdb_lastOutput]
	db_logMessage "opd.stdErr" $output

}

proc opd_dispatchGDB {} {

	set output  [gdb_lastOutput]
	db_logMessage "opd.gdb" $output


}

proc opd_dispatchStdOut {} {

	set output  [gdb_lastOutput]
	db_logMessage "opd.stdOut" $output


}

proc opd_dispatchOther {} {

	set output  [gdb_lastOutput]
	db_logMessage "opd.Unknwn" $output

	opd_setExit

}

proc opd_dispatchInterrupt {} {
	set output [gdb_lastOutput]
	db_logMessage "opd.timeout.gdb" $output
	
	gdb_getStackFrames

	set output [gdb_lastOutput]
	db_logMessage "opd.timeout.stack" $output

	after 15000 
	#wait 15 seconds before letting program die
	
}

proc opd_dispatchSigSeg {} {
	set output [gdb_lastOutput]
	db_logMessage "opd.SEGFAULT" $output
	gdb_getStackFrames
	set output [gdb_lastOutput]
	db_logMessage "opd.SEGFAULT" $output
	gdb_listLocals
        set output [gdb_lastOutput]
	db_logMessage "opd.SEGFAULT" $output

	opd_setExit
}

proc opd_setup {} {

	if { [ llength [ info procs user_setup ] ] > 0 } { 
		user_setup
	}
}

proc opd_dispatchPrgExt {} {
	set output [gdb_lastOutput]
	db_logMessage "opd.PrgExt" $output
	# Program automaticaly exits after this function
}

proc opd_processscript { filename } {
	gdb_setBreakpoint "main" "opd_breakscript $filename"
	set output [gdb_lastOutput]
        #db_logMessage "opd.processscript" $output
}

proc opd_breakscript { filename } {
	opd_readscript $filename
	gdb_continue
}

proc opd_readscript { filename } {
	global breakpoints
	global watchpoints
	set lastlocation ""
	set rank [ opd_getRank ]

	set f [ open $filename r ]
	while { [gets $f line] >= 0 } {
		#Process line
		set line [string trimleft $line]
		if { [string index $line 0] == "#" } {
			continue
		}
		if { [string index $line 0] == "@" } {
			#check to see if bp
			if { [ regexp {^@bp} $line ] == 1 } {
				# IS BP
                # These two lines strip the "@bp" part of the line
                # and leave only the location
				set items [ split $line " " ]
				set location [lindex $items 1]
				# set breakpoints(location,$location) $location
                # This creates an empty array that is populated in the loop below.
				set breakpoints(items,$location) {}
				# CRF - commented out 3 lines because of incorrect position
				#gdb_setBreakpoint $location "script_bp $location" 
				#set output [gdb_lastOutput]
				# db_logMessage "opd.breakpoint.set" $output

				if { $rank == 0} { db_logMessage "opd.spec.breakpoint.set" "$location" }
					if { [llength $items] > 2 } {
                        # Drop the first two elements "@bp" and "location"
                        # So that only the variable names are left, and then
                        # populate the empty array generated before with them
						set items [ lrange $items 2 [llength $items] ]
						set breakpoints(items,$location) $items
					if { $rank == 0} { 
						foreach it $breakpoints(items,$location) {
							db_logMessage "opd.spec.breakpoint.set.$location" "$it" 
						}
					}
                                }
                                # CRF 2012/07/03 - 3 lines added below
                                gdb_setBreakpoint $location "script_bp $location"
                                set output [gdb_lastOutput]
                                db_logMessage "opd.breakpoint.set" $output
                                
				set lastlocation $location
				continue
			}
			if { [ regexp {^@watch} $line ]== 1 } {
				# IS WATCH
				set items [ split $line " "]
				set location [lindex $items 1]
				# set watchpoints(location,$location) $location
				set item [lindex $items 2]
				set watchpoints(item,$location) $item
				gdb_setBreakpoint $location "script_bp_wa $location"	
				if { $rank == 0 } { db_logMessage "opd.spec.watchpoint.set" "$location = $item" }
				continue
                        }
		} else {
			# Else add to last breakpoint an expr
			set items $breakpoints(items,$lastlocation)
			lappend items $line
			set breakpoints(items,$lastlocation) $items
			if { $rank == 0 } {
				db_logMessage "opd.spec.breakpoint.set.$lastlocation" "$line"
			}
		}
	}
	close $f
	db_logMessage "opd.spec.done" "$f"
}

proc script_bp { x } {
	global breakpoints
	set output [gdb_lastOutput]
	db_logMessage "opd.spec.breakpoint.$x" $output

	# Get stack?
	gdb_getStackFrames
	set output [gdb_lastOutput]
	db_logMessage "opd.spec.breakpoint.$x" $output

	foreach item $breakpoints(items,$x) {
		set output [ gdb_evalExpr $item ]
			db_logMessage "opd.spec.breakpoint.$x.$item" $output
		}
	gdb_continue
}

proc script_bp_wa { x } {
	global breakpoints
	global watchpoints
	set item $watchpoints(item,$x)
	gdb_setWatchpoint "w" $item "script_wa_up $x"
	db_logMessage "opd.spec.watchpoint.start" "$x : $item"
	gdb_continue
}

proc script_wa_up { x } {
	global watchpoints
	set output [gdb_lastOutput]
	set item $watchpoints(item,$x)
	db_logMessage "opd.spec.watchpoint.$x.$item" $output
	gdb_continue
}
