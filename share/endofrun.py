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

import sys
from gdbase import *

#connect to database
def agent(jobid = None, database = None):
	db = GDBase()
	db.connect(database)

	# Obtain Job
	try:
		J = db.getJob(jobid)
	except:	
		print jobid, 'not found'
		sys.exit()
		return

	#print "OPD End of execution report"
	print "PBS JOBID: %s" % (J.getPBSID())
	print "DatabaseID: %s" % (J.getDBID())
	print "Statistics:"

	print "\tstart:\t%s" % J.getStartTime()
	print "\tend:\t%s" % J.getEndTime()

	print "\telapsed:\t%s" % J.getElapsedTime()

	print "\tncpus:\t\t%s" % J.getJobSize()

	M = J.getMessages()
	print "\tMessages:\t%s" % M.getCount()

	#Now run Detectors to "find" some errors

	#print "Agent Results: ",

if __name__ == "__main__":
	agent()

