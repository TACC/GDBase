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

import pgdb
import os

def parsegdb(str):
	gdboutput = str[str.find(',')+1:] #Slice off the reason from the front
	gdboutput = "{" + gdboutput.strip() + "}" #Add brackets for parsing


	(res, pos) = parse(gdboutput,0)
	return res


def parse(str, pos):
	# Check first element
	# If it is a [ then create a list, read each item with parse()
	# If it is a { then read each key ,=, parse()
	# If it is a "" then read to next quote, return as string
	if (str[pos] == '['): 
		res = list()
		mypos = pos + 1
		while str[mypos] != ']':
			# Check to see if this is some item= kind of list
			# If an = is before any next " [ { } ]  then skip to after the =
			# Find next non-alpha character
			nalpos = mypos
			while str[nalpos].isalpha():
				nalpos = nalpos + 1	
			if str[nalpos] == '=':
				mypos = nalpos + 1

			(resitm, respos) = parse(str,mypos)
			res.append(resitm)
			mypos = respos + 1
			if( str[mypos] == ',' ):
				mypos = mypos + 1
		return (res, mypos)
	if(str[pos] == '{'):
		res = dict()
		mypos = pos + 1
		while str[mypos] != '}':
			# Read key
			dakey = str[mypos: mypos + (str[mypos:].find('=') )] #from mypos to next =
			# Read value
			mypos = mypos + (str[mypos:].find('=')) + 1
			(davalue, dapos) = parse(str, mypos)
			mypos = dapos + 1
			if( str[mypos] == ',' ):
                                mypos = mypos + 1


			# Attach
			res[dakey] = davalue
		return (res, mypos)
	if(str[pos] == '"'):
		mypos = pos + 1
		res = str[mypos: mypos + (str[mypos:].find('"') )] #find next "
		mypos = mypos + str[mypos:].find('"') 
		return (res,mypos)
	return ("ERROR", pos)

def findIndex(c, name):
	# C is cursor
	# name is name of column to find
	f = c.description
	for i in range(len(f)):
		if f[i][0] == name:
			return i
	return None

def printstack(stack):
	stk = stack['stack']
	stk.reverse()
	print 'Stack: Level\tFunction\t\tFile    : Line'
	for k in stk:
		level = '-'
		func = '-'
		file = '-'
		line = '-'

		if (k.keys().count('level') >0): level = k['level']
		if (k.keys().count('func') >0): func = k['func']
		if (k.keys().count('file') >0): file = k['file']
		if (k.keys().count('line') >0): line = k['line']
		print "\t%s\t%s\tin\t%s:%s" % ( level,  func, file, line)

		if func.startswith('PMPI') or func.startswith('MPI'):
			print "\t --- "
			break;



class GDBase:
	def __init__(self):
		self.db = None
		return
	def connect(self, database = None):
		# Parse connection details from relevant locations
		# self.db = pg.connect(dbname='OPD', host='localhost', user='opd', passwd='opd123')
		# self.db = pgdb.connect('localhost:OPD:opd:opd123')
		# self.db = pgdb.connect(host = dbdict['host'], database = dbdict['dbname'], user = dbdict['user'], password = dbdict['password'])
		self.db = pgdb.connect(database)
		return
	def getJob(self, name = None):
		if name == None and os.environ.keys().count('PBS_JOBID') > 0:
			name = os.environ['PBS_JOBID'];
		
		if name == None:
			return None

		# Create new Job object, return it
		J = Job(name, self.db)
		return J
	def getCursor(self):
		return self.db.cursor()

class Job:
	def __init__(self, name, db):
		self.db = db
		self.name = name

		cur = db.cursor()
		cur.execute("select * from jobs where queueid = '%s'" % name)

		idIndex = findIndex(cur,'id')
		res = cur.fetchone()
		self.jobid = res[idIndex]
		return
	def getPBSID(self):
		return self.name
	def getDBID(self):
		return self.jobid
	def getStartTime(self):
		cur = self.db.cursor()
		cur.execute("select min(tstamp) from messages where job_id=%d"  % (self.jobid) )

		idMin = findIndex(cur,'min')
		res = cur.fetchone()
		starttime = res[idMin]

		return starttime
	def getEndTime(self):
		cur = self.db.cursor()
		cur.execute("select max(tstamp) from messages where job_id=%d"  % (self.jobid) )

		idMax = findIndex(cur, 'max')
		res = cur.fetchone()
		endtime = res[idMax]

		return endtime
	def getElapsedTime(self):
		cur = self.db.cursor()
		cur.execute("select max(tstamp) - min(tstamp) AS elapsed from messages where job_id=%d"  % (self.jobid)  )

		idElapsed = findIndex(cur,'elapsed')
		res = cur.fetchone()
		elapsed = res[idElapsed]

		return elapsed
	def getJobSize(self):
		cur = self.db.cursor()
		cur.execute("select max(rank) from messages where job_id=%d"  % (self.jobid) )

		idMax = findIndex(cur, 'max')
		res = cur.fetchone()
		ncpus = res[idMax] + 1

		return ncpus
	def getMessages(self):
		#Create new messages object with only job filter set
		M =  Messages(self.db, self.jobid)
		return M
	def getBreakpoints(self):
		B = Breakpoints(self)
		return B

	def getWatchpoints(self):
		W = Watchpoints(self)
		return W
	

class Messages:
	def __init__(self, db, jobid):
		self.db = db
		self.jobid = jobid
		self.cursor = db.cursor()
		self.conditions={}
		self.changed = True
		self.limit = None
		self.reverse = False

		self.clear()

		return
	def getNext(self): 
		if self.changed:
			self.update()

		res = self.cursor.fetchone()
		hash = {}

		if res == None:
			return None

		for i in range(len(res)):
			hash[ self.cursor.description[i][0] ] = res[i]

		return hash
	def getAll(self):
		res = []
		for r in self:
			res.append(r)

		return res

	def _toSQL(self, val):
		return "'" + str(val) + "'"
		
	def update(self):
		# Execute the cursor
		whereclause = ""
		for cond in self.conditions.values():
			if(len(cond) == 2):
				whereclause = whereclause + (cond[0]) + " = " + self._toSQL(cond[1])
			if(len(cond) == 3):
				whereclause = whereclause + (cond[0]) + " " + (cond[1]) + " " + self._toSQL(cond[2])

			whereclause = whereclause + (" and ")

		#delete last and from cond
		whereclause = whereclause[:-5]
		sql = ''
		if len(self.conditions) > 0:
			sql = "SELECT * FROM MESSAGES WHERE " + whereclause + " ORDER BY ID ASC"
		else:
			sql = "SELECT * FROM MESSAGES ORDER BY ID ASC"

		if self.reverse:
			sql = sql[:-4] # Remove ASC
			sql = sql + " DESC"

		if self.limit != None:
			sql = sql + (" LIMIT %d" % self.limit)
		
		self.cursor.execute(sql)

		self.changed = False
	def clear(self):
		self.conditions = {}
		self.conditions['job_id'] = ('job_id', self.jobid)
		self.limit = None
		self.reverse = False

		self.changed = True

	def setRank(self, rank):
		self.conditions['rank'] = ('rank',rank)
		self.changed = True

	def setKey(self, key):
		self.conditions['key'] = ('key','LIKE',key + "%%")
		self.changed = True
	def setKeyExact(self, key):
		self.conditions['key'] = ('key', '=', key)
		self.changed = True
	def setValue(self, val):
		self.conditions['value'] = ('value', 'LIKE', "%%" + val + "%%")
		self.changed = True
	def setLimit(self, limit):
		self.limit = limit
		self.changed = True
	def setID(self, comp, id):
		self.conditions['id'] = ('id', comp, id)
		self.changed = True
	def setReverse(self):
		self.reverse = True
		self.changed = True

	def getCount(self):
		if self.changed:
			self.update()
		return self.cursor.rowcount 
	def __iter__(self):
		return self
	def next(self):
		res = self.getNext()
		if res == None:
			raise StopIteration
		return res

class Breakpoints:
	def __init__(self, myjob ):
		self.myjob = myjob
		self.bpoints = {}
		self.messages = myjob.getMessages()

		self._setup()

	def setRank(self, rank):
		self.reset()
		self.messages.setRank(rank)

	def _setup(self):
		M = self.messages # Copy for conveince
		M.setRank(0)
		M.setKey('opd.spec.breakpoint.set')
		
		for m in M:
			if m['key'] == 'opd.spec.breakpoint.set':
				self.bpoints[ m['value'] ] = []
				continue
			
			loc = m['key'].split('.')[-1] # Last .something is location
			self.bpoints[loc].append( m['value'] )

		self.reset()

	def reset(self):
		# Reset us for iteration!
		M = self.messages
		M.clear()
		M.setKey('opd.spec.breakpoint')

	def getDescription(self):
		return self.bpoints

	def __iter__(self):
		return self

	def next(self):
		M = self.messages

		msg = M.getNext() #this will throw out of data exception for us!
		while ( msg != None and ( msg['key'].startswith('opd.spec.breakpoint.set') or msg['key'].startswith('opd.spec.done') ) ):
			msg = M.getNext() # Move past all 'sets' this will throw outofdata exception for us!

		if msg == None:
			raise StopIteration			

		#Figure out which breakpoint we have
		loc = msg['key'].split('.')[-1]
		gdb = parsegdb(msg['value'])

		# Set a filtered message up, we only want messages after this one (should be the next few)
		FM = self.myjob.getMessages()
		FM.setRank(msg['rank'])
		FM.setID('>', msg['id'])
		FM.setKey('opd.spec.breakpoint.%s' % loc)

		bpvals = {}

		for bps in self.bpoints[loc]:
			FM.setKey( 'opd.spec.breakpoint.%s.%s' % (loc,bps) )
			i = FM.getNext()
			# Get which value was sampled
			val = i['key'].split('.')[-1]
			bpvals[ val ] = i['value']

			# move M to the last ID we pulled
			M.setID('>', i['id'])
		
		return (loc, gdb, bpvals) 

class Watchpoints:
	def __init__(self, myjob):
		self.myjob = myjob
		self.messages = myjob.getMessages()
		self.wpoints = []

		self._setup()

	def setRank(self, rank):
                self.reset()
                self.messages.setRank(rank)

        def _setup(self):
                M = self.messages # Copy for conveince
                M.setRank(0)
                M.setKey('opd.spec.watchpoint.set')
		
		# Parse them
		for m in M:
			a = m['value'].split('=')
			loc = a[0].strip()
			val = a[1].strip()
			self.wpoints.append( (loc,val) )

                self.reset()

	def getDescription(self):
		return self.wpoints

        def reset(self):
                # Reset us for iteration!
                M = self.messages
                M.clear()
                M.setKey('opd.spec.watchpoint')

	def __iter__(self):
		return self

	def next(self):
		M = self.messages

		msg = M.getNext()
		while( msg != None and (msg['key'].startswith('opd.spec.watchpoint.set') or msg['key'].startswith('opd.spec.watchpoint.start')) ):
			msg = M.getNext()

		if msg == None:
			raise StopIteration

		key = msg['key'].split('.')
		val = key[-1]
		loc = key[-2]

		gdb = parsegdb(msg['value'])

		return (loc,val,gdb)	

