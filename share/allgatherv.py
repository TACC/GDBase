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
# Copyright 2012 Robert McLay, Carlos Rosales, Doug James
#
# **********************************************************************
#
# 24 Apr 2012 -- Changed name from allgather.py to allgatherv.py (DJ)
#                Added print statements reporting no errors found. (DJ)
#
# **********************************************************************



# AllGatherv interpreter, Checker

from gdbase import *

class Allgathervs:
	def __init__(self, myjob):
		self.myjob = myjob
		self.agvs = 0
		self.curr_agvs = -1

		self._setup()
	
	def _setup(self):
		# Get Number of Allgathervs Performed
		M = self.myjob.getMessages()
		M.setRank(0)
		M.setKey('allgatherv.catch.')
		
		self.agvs = M.getCount()

	def getCurrentCount(self):
		return self.curr_agvs
	def getTotalCount(self):
		return self.agvs
	def getNext(self):

		res = []
		self.curr_agvs = self.curr_agvs + 1

		if self.curr_agvs >= self.agvs:
			return None

		M = self.myjob.getMessages()

		size = self.myjob.getJobSize()
	
		for i in range(size):
			r = {}
			M.setRank(i)
			M.setKeyExact('allgatherv.catch.%d' % self.curr_agvs)
			m = M.getNext()

			r['gdb'] = m['value']

	
			M.setKeyExact('allgatherv.%d.sendcount' % self.curr_agvs)

			m = M.getNext()

			r['sendcount'] = m['value']
		
			r['recvcount'] = []
			r['displs'] = []

			for j in range(size):
				M.setKeyExact('allgatherv.%d.recvcount.%d' % (self.curr_agvs, j) )
				m = M.getNext()
				r['recvcount'].append(m['value'])

				M.setKeyExact('allgatherv.%d.displs.%d' % (self.curr_agvs, j) )
				m = M.getNext()
				r['displs'].append(m['value'])


			res.append(r)

		return res

        def __iter__(self):
                return self
        def next(self):
                res = self.getNext()
                if res == None:
                        raise StopIteration
                return res


def agent(name = None, database = None):
	db = GDBase()
	db.connect(database)
	
	J = db.getJob(name)

	if J == None:
		print 'Job not found'
		return

	A = Allgathervs(J)
	count = 0
        totalerrors = 0
	for a in A:
	        print "   ...checking allgatherv occurrence %d..." % count
		count = count + 1
		# Check for send-> recv equality
		send = []
		for i in a:
			send.append(i['sendcount'])

		for i in range(len(a)):
			error = []
			for j in range(len(send)):
				if send[j] != a[i]['recvcount'][j]:
					error.append(j)

			if len(error) > 0:
				print "Error at element %d in recvcount array on task %d" % (error[0],i)					
				print "was %s but should be %s" % (a[i]['recvcount'][ error[0] ], send[ error[0] ])
				gdb = parsegdb(a[i]['gdb'])
				printstack(gdb)
                                totalerrors = totalerrors + 1

		# Should check each task for displ non-overlap. 
		# Create bit field of size sum(displs)
		# go through, coloring by recvcount[i] at displs[i]
			# if already colored, print error

        if totalerrors == 0:
		print "Checked MPI_Allgatherv send/receive count equality in all calls and all processes; no errors detected."
		print "Did not check offsets against recvcounts; this functionality not yet implemented."



if __name__ == "__main__":
	agent(None)
