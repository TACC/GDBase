#!/usr/bin/python
# -*- python -*-
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


# Detects and reports a segfault
from gdbase import *

#connect to database
def agent(jobid = None, database = None):
	db = GDBase()
	db.connect(database)

	J = db.getJob(jobid)
	if J == None:
		print "Job not found"
		return

	ncpus = J.getJobSize()

	W = J.getWatchpoints()

	#For each rank
	for i in range(0,ncpus):
		W.reset()
		W.setRank(i)

		trace = []

		for w in W:
			# Append Data to Trace
			gdb = w[2]
			n = {}
			n['file'] = gdb['frame']['file']
			n['line'] = gdb['frame']['line']

			n['old'] = gdb['value']['old']
			n['new'] = gdb['value']['new']

			trace.append( n )

			if len(trace) > 20:
				trace = trace[1:]	

			if n['new'].find('nan') >= 0:
				#Found an error, print it!
				print "NaN found on rank %d point %s expression %s" % (i, w[0], w[1] )
				print"File : Line \t\tNew\t-\tOld"	
				for n in trace:
					print "%s : %s \t\t%s\t-\t%s" % (n['file'],n['line'],n['new'],n['old'])
				break

if __name__ == "__main__":
	agent()
					
