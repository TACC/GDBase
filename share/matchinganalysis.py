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
# Copyright 2012 Doug James, Carlos Rosales, Dan Stanzione

# Detects and reports multiple instances of mathing arguments in 
# Isend and Irsend messages.

from gdbase import *

#connect to database

def agent(jobid = None, database = None):
	db = GDBase()
	db.connect(database)

	# obtain job number from environment
	J = db.getJob(jobid)
	if J == None:
		print 'MATCHARGS: Job not found'
		return

	# set error flag to zero
	ERROR = 0
	ncpus = J.getJobSize()

	# define two objects with the subset of db entries
	# that match the SQL key "agent.%"
	Mr  = J.getMessages()
	Mir  = J.getMessages()
	Nb1is = J.getMessages()
	Nb1irs = J.getMessages()
	Nb2is = J.getMessages()
	Nb2irs = J.getMessages()
	Mr.setKey('agent.recv')
	Mir.setKey('agent.irecv')
	Nb1is.setKey('agent.isend')
	Nb2is.setKey('agent.isend')
	Nb1irs.setKey('agent.irsend')
	Nb2irs.setKey('agent.irsend')

	for i in range(0,ncpus):
		# Check if the first error has been found
		if ERROR == 1:
			break

		# These are the expected messages
		Mr.setRank(i)
		Mir.setRank(i)
		resMr = Mr.getAll()
		resMir = Mir.getAll()

		# These are the messages sent by nearest neighbors
		nb1 = i-1
		Nb1is.setRank(nb1)
		Nb1irs.setRank(nb1)
		resNb1is = Nb1is.getAll()
		resNb1irs = Nb1irs.getAll()
		nb2 = i+1
		Nb2is.setRank(nb2)
		Nb2irs.setRank(nb2)
		resNb2is = Nb2is.getAll()
		resNb2irs = Nb2irs.getAll()

		# Check all messages completed by the Irecv calls
		for j in resMir:
			if ERROR == 1:
				break
			typeR = "IRECV"
			recvRank = i
			valueMir = parsegdb(j['value'])
			if 'stack' in valueMir:
				stackR = valueMir
			if 'rank' in valueMir:
				tagMir  = int(valueMir['tag'])
				sizeMir = int(valueMir['size'])
				recvSize = sizeMir
				for n in resNb1is:
					if ERROR == 1:
						break
					typeS = "ISEND"
					sendRank = nb1
					valueNb1is = parsegdb(n['value'])
					if 'stack' in valueNb1is:
						stackS = valueNb1is
					if 'rank' in valueNb1is:
						destNb1is = int(valueNb1is['dest'])
						tagNb1is  = int(valueNb1is['tag'])
						sizeNb1is = int(valueNb1is['size'])
						# Here check for matches
						if i == destNb1is and tagMir == tagNb1is:
							if sizeMir != sizeNb1is:
								sendSize = sizeNb1is
								ERROR = 1
								break
				for n in resNb2is:
					if ERROR == 1:
						break
					typeS = "ISEND"
					sendRank = nb2
					valueNb2is = parsegdb(n['value'])
					if 'stack' in valueNb2is:
						stackS = valueNb2is
					if 'rank' in valueNb2is:
						destNb2is = int(valueNb2is['dest'])
						tagNb2is  = int(valueNb2is['tag'])
						sizeNb2is = int(valueNb2is['size'])
						# Here check for matches
						if i == destNb2is and tagMir == tagNb2is:
							if sizeMir != sizeNb2is:
								sendSize = sizeNb2is
								ERROR = 1
								break
				for n in resNb1irs:
					if ERROR == 1:
						break
					typeS = "IRSEND"
					sendRank = nb1
					valueNb1irs = parsegdb(n['value'])
					if 'stack' in valueNb1irs:
						stackS = valueNb1irs
					if 'rank' in valueNb1irs:
						destNb1irs = int(valueNb1irs['dest'])
						tagNb1irs  = int(valueNb1irs['tag'])
						sizeNb1irs = int(valueNb1irs['size'])
						# Here check for matches
						if i == destNb1irs and tagMir == tagNb1irs:
							if sizeMir != sizeNb1irs:
								sendSize = sizeNb1irs
								ERROR = 1
								break

				for n in resNb2irs:
					if ERROR == 1:
						break
					typeS = "IRSEND"
					sendRank = nb2
					valueNb2irs = parsegdb(n['value'])
					if 'stack' in valueNb2irs:
						stackS = valueNb2irs
					if 'rank' in valueNb2irs:
						destNb2irs = int(valueNb2irs['dest'])
						tagNb2irs  = int(valueNb2irs['tag'])
						sizeNb2irs = int(valueNb2irs['size'])
						# Here check for matches
						if i == destNb2irs and tagMir == tagNb2irs:
							if sizeMir != sizeNb2irs:
								sendSize = sizeNb2irs
								ERROR = 1
								break

		# Check all messages completed by the Recv calls
		for j in resMr:
			if ERROR == 1:
				break
			typeR = "RECV"
			valueMr = parsegdb(j['value'])
			if 'stack' in valueMr:
				stackR = valueMr
			if 'rank' in valueMr:
				tagMr  = int(valueMr['tag'])
				sizeMr = int(valueMr['size'])				
				recvSize = sizeMr

				for n in resNb1is:
					if ERROR == 1:
						break
					typeS = "ISEND"
					sendRank = nb1
					valueNb1is = parsegdb(n['value'])
					if 'stack' in valueNb1is:
						stackS = valueNb1is
					if 'rank' in valueNb1is:
						tagNb1is  = int(valueNb1is['tag'])
						sizeNb1is = int(valueNb1is['size'])
						# Here check for matches
						if tagMr == tagNb1is:
							if sizeMr != sizeNb1is:
								sendSize = sizeNb1is
								ERROR = 1
								break
				for n in resNb2is:
					if ERROR == 1:
						break
					typeS = "ISEND"
					sendRank = nb2
					valueNb2is = parsegdb(n['value'])
					if 'stack' in valueNb2is:
						stackS = valueNb2is
					if 'rank' in valueNb2is:
						tagNb2is  = int(valueNb2is['tag'])
						sizeNb2is = int(valueNb2is['size'])
						# Here check for matches
						if tagMr == tagNb2is:
							if sizeMr != sizeNb2is:
								sendSize = sizeNb2is
								ERROR = 1
								break
				for n in resNb1irs:
					if ERROR == 1:
						break
					typeS = "IRSEND"
					sendRank = nb1
					valueNb1irs = parsegdb(n['value'])
					if 'stack' in valueNb1irs:
						stackS = valueNb1irs
					if 'rank' in valueNb1irs:
						tagNb1irs  = int(valueNb1irs['tag'])
						sizeNb1irs = int(valueNb1irs['size'])
						# Here check for matches
						if tagMr == tagNb1irs:
							if sizeMr != sizeNb1irs:
								sendSize = sizeNb1irs
								ERROR = 1
								break
				for n in resNb2irs:
					if ERROR == 1:
						break
					typeS = "IRSEND"
					sendRank = nb2
					valueNb2irs = parsegdb(n['value'])
					if 'stack' in valueNb2irs:
						stackS = valueNb2irs
					if 'rank' in valueNb2irs:
						tagNb2irs  = int(valueNb2irs['tag'])
						sizeNb2irs = int(valueNb2irs['size'])
						# Here check for matches
						if tagMr == tagNb2irs:
							if sizeMr != sizeNb2irs:
								sendSize = sizeNb2irs
								ERROR = 1
								break
				


	if ERROR == 0:
		print "\tNo problems detected"
	if ERROR == 1:
		print '\n'
		print '*** Mismatched arguments found in %s / %s completion.\n' %(typeR,typeS)
		print '    Instance 1:'
		print '        Receiving rank was %d and sending rank was %d' % (recvRank,sendRank)
		print '        Expected size was %d but received size was %d' % (recvSize,sendSize)
		print '\nReceiving rank stacktrace:'
		printstack(stackR)
		print '\nSending rank stacktrace:'
		printstack(stackS)

		
if __name__ == "__main__":
        agent(None)
