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

def addComm(msg_id, rank, type, bytes, remote, tag):
	cur.execute("INSERT INTO comm (msg_id, rank, type, bytes, remote, tag) VALUES ('%s', '%s', '%s', '%d', '%s', '%s');" % (msg_id, rank, type,bytes,remote,tag) )

def getNext(cursor):
	res = cursor.fetchone()
	hash = {}

	if res == None:
		return None

	for i in range(len(res)):
		hash[ cursor.description[i][0] ] = res[i]

	return hash	

def printMessage(msg):
	d = db.getCursor()
	d.execute("select * from messages where id = '%s';" % msg['msg_id'])

	row = getNext(d)
	#Extrac type from key
	key = row['key'].split('.')

	type = key[1]
	rank = msg['rank']
	remote = msg['remote']

	if(msg['type'] == 'S'):
		print '%s : %s ---> %s' % (type, rank, remote)
	if(msg['type'] == 'R'):
		print '%s : %s <--- %s' % (type, rank, remote)

def printfinalstack(rank):
        M = J.getMessages()
        M.setRank(rank)
        M.setReverse()
        M.setKeyExact('opd.timeout.stack')
        M.setLimit(1)

        m = M.getNext()
        gdb = parsegdb(m['value'])

	print 'Final Stack for Rank %s' % rank
	printstack(gdb)        


def agent(jobid = None, database = None):
	#connect to database

	db = GDBase()
	db.connect(database)

	J = db.getJob(jobid)
	if J == None:
		print "Job not found"
		return

	ncpus = J.getJobSize()

	communication = []


	# Extract every communication
	comm_types = [['R','Recv'], ['R','Irecv'], ['S','Send'], ['S','Isend']]

	#Create temporary table
	cur = db.getCursor()
	cur.execute('CREATE TEMP TABLE comm (id serial primary key, msg_id integer, rank integer, type character(1), bytes integer, remote integer, tag integer, flag boolean default false);')
	cur.execute('CREATE INDEX comm_idx on comm (type,bytes,remote,tag);')
	cur.execute('create INDEX comm_flag_idx on comm(flag);')


	for i in range(0,ncpus):
		M = J.getMessages()
		M.setRank(i)
		
		for type in comm_types:
			M.setKey('deadlock.%s.values' % type[1])

			for m in M:
				values = parsegdb(m['value'])
				addComm(m['id'], i, type[0], int(values['count'])*int(values['size']), values['remote'], values['tag'])

	# Cleanup communications by matching
	lastid = 0
	while(True):
		if(lastid >0):	
			cur.execute('select * from comm where id > %d and flag = False order by id asc limit 1;' % lastid)
		else:
			cur.execute('select * from comm where flag = False order by id asc limit 1;')
		res = getNext(cur)
		
		# if None break
		if(res == None):
			break

	#	print 'A: ',
	#	print res
		lastid = res['id']
		# Find corresponding
		srch = db.getCursor()

		corresponding = 'R'
		if(res['type'] == 'R'):
			corresponding = 'S'

		srch.execute("select * from comm where flag = False and type = '%s' and bytes = '%s' and remote = '%s' and tag = '%s' order by id limit 1;" % (corresponding, res['bytes'], res['rank'], res['tag']) )
		if srch.rowcount > 0:
		#if found
			rmt = getNext(srch)
			mark = db.getCursor()

	#		print 'D: ',
	#		print rmt
			#set to false
			mark.execute("update comm set flag = True where id = '%s';" % rmt['id'])
			#set this to false
			mark.execute("update comm set flag = True where id = '%s';" % res['id'])

		

	# Delete all messages with Flag = true
	cur.execute("delete from comm where flag = True;")
	cur.execute("select * from comm order by id asc;")

	while(True):
		res = getNext(cur)
		if(res == None):
			break
		
		communication.append( (res['type'], res['rank'], res['remote'], res['id']) )

	if len(communication) > 0:
		# We have a problem
		# Find node with maximum dependence
		edges = []
		for c in communication:
			if c[0] == 'S':
				edges.append( (c[1],c[2]) )
			if c[0] == 'R':
				edges.append( (c[2],c[1]) )

		nodes = range(0,ncpus)
		nodes2 = copy.copy(nodes)
		checked = []
		weights = [-1]*ncpus
		i = -1
		while weights.count(i) > 0:
			i = i + 1
			for n in nodes:
				# Check to see if any edges end here that start from nodes 
				cont = True
				for e in edges:
					if nodes.count( e[0] ) >0 and e[1] == n:
						cont = False
						break
				if cont:
					weights[n] = i
					nodes2.remove(n)
					checked.append(n)
			nodes = copy.copy(nodes2)

		print "Incomplete Communication"
		first = -1
				
		while i > 1:	
			i = i - 1
			rank = weights.index(i)
			if (first < 0): first = rank

			cur.execute("select * from comm where rank = '%d' order by id asc limit 10;" % rank)
			while(True):
				res = getNext(cur)
				if(res == None):
					break
				printMessage(res)

				printfinalstack(rank)
		
			
	#		M = J.getMessages()
	#		M.setRank(rank)
	#		M.setKeyExact('opd.timeout.stack')	
	#		m = M.getNext()
	#		stack = parsegdb(m['value'])
	#		print 'Order %d\t Rank %d' % (i, rank)
	#		printstack(stack)

if __name__ == "__main__":
	agent()
