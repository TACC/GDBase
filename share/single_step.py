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

	try:
		J = db.getJob(jobid)
	except:	
		print jobid, 'not found'
		sys.exit()
		return

	M = J.getMessages()
	
	locals = []
	stack = []

	M.setKey('opd.step.local')
	if M.getCount() > 0:
		steps = M.getAll()
		for s in steps:
			value = parsegdb(s['value'])
			locals.append(value['locals'])

	M.setKey('opd.step.stack')
	if M.getCount() > 0:
		steps = M.getAll()
		for s in steps:
			value = parsegdb(s['value'])
			stack.append(value['stack'])

	print len(locals)
	print len(stack)

	#for i in range(0,9):
	for i in stack:
		#print locals[i]
		#print i[0]['func']
		#if i[0]['func'] != 'main':
		#	print i
		if len(i) > 1:
			print i

if __name__ == "__main__":
        agent(None)
