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

	# obtain job number from environment
	J = db.getJob(jobid)
	if J == None:
		print 'SEGREPORT: Job not found'
		return

	M = J.getMessages()
	M.setKey('opd.SEGFAULT')

	if M.getCount() > 0:
		res = M.getNext()
		print "Job crashed on Rank: %d" % res['rank'], " Thread:",
		gdb = parsegdb(res['value'])
		print gdb['thread-id']
		print "At:"
		print "%s\tin\t%s:%s" % ( gdb['frame']['func'], gdb['frame']['file'], gdb['frame']['line'])
		print ""
		print "With stack:"
		res = M.getNext()
		gdb = parsegdb(res['value'])	
		printstack(gdb)

		print ""
		print "With locals:"
		res = M.getNext()
		gdb = parsegdb(res['value'])
		for i in gdb['locals']:
			print ("%s\t%s\t=\t%s") % (i['type'], i['name'], i['value'])

	else:
		print "No Segfault"

		
if __name__ == "__main__":
        agent(None)
