#!/usr/bin/python
#-*- encoding: utf-8 -*-


import sys, os, socket, struct, binascii



WAITSECS = 30
WAITMILSECS = 0



class CNetwork (object):
	"""The class for network operations. (thanks cap !)"""
	headerSize = 256
	MaxDataLen = 128 * 1024 * 1024
	intLen = 4

	ExecuteCommand = 0x0
	ServerAnswer = 0x1
	ServerRequest = 0x2
	ClientAnswer = 0x4
	ClientRequest = 0x8
	#
	CommandsSum = 0x0+0x1+0x2+0x4+0x8

	Positive = 0x0
	Negative = 0x1
	CanContinue = 0x2
	#
	StatusesSum = 0x0+0x1+0x2

	NoStatus = 0
	BadName = 1
	BadPass = 2
	InternalServerError = 3
	TooLong = 4
	InteractionFin = 5
	RunAsUser = 6
	
	def __init__ (self, hostStr, portStr, waitSec, waitMilSec):
		if 1 != 1:
			print ("Hello Crotchy !")
		
		self.sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
		timeVal = struct.pack ("LL", waitSec, 1000 * waitMilSec)
		self.sock.setsockopt (socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeVal)
		self.sock.setsockopt (socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeVal)
		# connect method accepts a tuple, not 2 parameters
		self.sock.connect ((hostStr, int (portStr)))
		
		return
	
	def __del__ (self):
		self.close ()
	
	def close (self):
		self.sock.close ()
	
	def SendString (self, parsLst):
		# strDat, cmdType, retStatus, retExtraStatus - in parsLst
		
		data = struct.pack ("i", len (parsLst[0]))
		data += struct.pack ("i", parsLst[1])
		data += struct.pack ("i", parsLst[2])
		data += struct.pack ("i", parsLst[3])
		data += struct.pack ("B", 0) * (CNetwork.headerSize - len (data))
		data += parsLst[0]
		#print binascii.hexlify (data)
		if __debug__:
			if self.CheckFormatHeader ([len (parsLst[0]), parsLst[1], parsLst[2], parsLst[3]]) != 0:
				raise Exception ("Uncorrect format of a header at SendString")
		
		totLen = 0
		while totLen < len (data):
			ret = self.sock.send (data[totLen : len (data)])
			totLen += ret
			
		return totLen - CNetwork.headerSize
	
	
	def ReadString (self):
		dataLst = []
		totLen = 0
		
		while True:
			ret = self.sock.recv (CNetwork.headerSize - totLen)
			totLen += len (ret)
			dataLst.append (ret)
			if totLen == CNetwork.headerSize: break
		data = "".join (dataLst)
		#print binascii.hexlify (data)
		
		totLen = struct.unpack ("i", data[0:4])[0]
		cmdType = struct.unpack ("i", data[4:8])[0]
		retStatus = struct.unpack ("i", data[8:12])[0]
		retExtraStatus = struct.unpack ("i", data[12:16])[0]
		
		ret = self.CheckFormatHeader ([totLen, cmdType, retStatus, retExtraStatus])
		if 0 != ret:
			print ("Not correct header, ret of CheckHeader: {par}".format (par = ret))
			return -1, cmdType, retStatus, retExtraStatus
		
		dataLst = []
		while totLen > 0:
			ret = self.sock.recv (totLen)
			totLen -= len (ret)
			dataLst.append (ret)
		data = "".join (dataLst)
			
		return [data, cmdType, retStatus, retExtraStatus]
	
	
	def CheckFormatHeader (self, hdrInternalsList):
		# len, cmdType, status, extraStatus
		lenData, cmdType, retStatus, retExtraStatus = hdrInternalsList
		
		#if __debug__:
		#	print ("Values of header's fields: len: {0}, cmd: {1}, status: {2}, extra-"
		#		   "status: {3}".format (lenData, cmdType, retStatus, retExtraStatus))
		
		if cmdType < CNetwork.ExecuteCommand or cmdType > CNetwork.CommandsSum:
			print ("Not correct cmd type")
			return 2
		
		if retStatus < CNetwork.Positive or retStatus > CNetwork.StatusesSum:
			print ("Not correct ret status")
			return 3
		
		if retExtraStatus < CNetwork.NoStatus or retExtraStatus > CNetwork.InteractionFin:
			print ("Not correct ret extra status")
			return 4
		
		return 0
	
	
	def CheckLogicallyHeader (self, hdrInternalsList):
		# len, cmdType, status, extraStatus
		lenData, cmdType, retStatus, retExtraStatus = hdrInternalsList
		
		if lenData > CNetwork.MaxDataLen:
			#print ("Have received data too long: {bts} bytes".format (bts = lenData))
			return 1
		
		if (retStatus & CNetwork.Negative) and (not (retStatus & CNetwork.CanContinue)):
			#print ("Server has returned fatal error, we can't continue processing, status:"
			#	   " {0}, extra-status: {1}". format (retStatus, retExtraStatus))
			return 2
		
		return 0
	
	
	
class CApplication (object):
	"""The class for data interchange with the server"""
	def __init__ (self):
		self.one = 1
	
	
	def AuthInfoTransfer (self, netObj, userName, passStr):
		# first string from server
		ret = netObj.ReadString ()
		if -1 == ret[0]:
			print ("Format error of header")
			return 1
		if 0 != netObj.CheckLogicallyHeader ([len (ret[0])] + ret [1 : len (ret)]):
			return 2
		sys.stdout.write (ret[0])
		
		# login name
		sndStr = sys.stdin.readline ()
		netObj.SendString (
			[sndStr[0 : len (sndStr) - 1],
			CNetwork.ClientAnswer,
			CNetwork.Positive,
			CNetwork.NoStatus]
		)
		ret = netObj.ReadString ()
		if -1 == ret[0]:
			print ("Format error of header")
			return 1
		if 0 != netObj.CheckLogicallyHeader ([len (ret[0])] + ret [1 : len (ret)]):
			print ("Not correct name")
			return 2
		sys.stdout.write (ret[0])
		
		# password
		sndStr = sys.stdin.readline ()
		netObj.SendString (
			[sndStr[0 : len (sndStr) - 1],
			CNetwork.ClientAnswer,
			CNetwork.Positive,
			CNetwork.NoStatus]
		)
		ret = netObj.ReadString ()
		if -1 == ret[0]:
			print ("Format error of header")
			return 1
		if 0 != netObj.CheckLogicallyHeader ([len (ret[0])] + ret [1 : len (ret)]):
			print ("Not correct password")
			return 2
		sys.stdout.write (ret[0])
		
		return 0
	
	
	def WorkerLoop (self, netObj):
		valCmd = 0; valStatus = 0; valExtrStatus = 0
		
		while True:
			# to send a command
			sndStr = sys.stdin.readline ()
			if sndStr[0 : len (sndStr) - 1] == "exit":
				netObj.SendString (
					[sndStr[0 : len (sndStr) - 1],
					CNetwork.ClientAnswer,
					CNetwork.Positive,
					CNetwork.NoStatus]
				)
				sys.stdout.write ("Results: {strr}".format (strr = netObj.ReadString ()[0]))
				return
			else if sndStr[0 : len ("asuser:")] == "asuser:":
				colonInd = "".find (sndStr, ':') + 1
				spaceInd = "".find (sndStr, ' ')
				
				# Здесь нужно использовать регулярные выражения !
				
				netObj.SendString (
					[sndStr[len ("asuser:") : len (sndStr) - 1] + sndStr[colonInd : spaceInd],
					CNetwork.ClientAnswer | CNetwork.ClientRequest,
					CNetwork.Positive,
					CNetwork.RunAsUser]
				)
			else:
				netObj.SendString (
					[sndStr[0 : len (sndStr) - 1],
					CNetwork.ClientAnswer | CNetwork.ClientRequest,
					CNetwork.Positive,
					CNetwork.NoStatus]
				)
			
			#sys.stdout.write ("Results: ")
			ret = netObj.ReadString ()
			if -1 == ret[0]:
				print ("Format error of header")
				return
			if 0 != netObj.CheckLogicallyHeader ([len (ret [0])] + ret [1 : len (ret)]):
				return
			sys.stdout.write ("Results: {strr}".format (strr = ret[0]))
		
		return
	


def main ():
	if len (sys.argv) < 3:
		print ("Need host and port parameters")
		return
	if __debug__:
		print ("Connecting to: {0}, at port: {1}".format (sys.argv[1], sys.argv[2]))
	
	
	netObj = CNetwork (sys.argv[1], sys.argv[2], WAITSECS, WAITMILSECS)
	applObj = CApplication ()
	
	if 0 != applObj.AuthInfoTransfer (netObj, "", ""):
		return
	applObj.WorkerLoop (netObj)
		
	print ("Bye bye")
	
	
	return
	


if __name__ == "__main__":
	main ()
else:
	print ("This script can't run as a module")
	raise Exception ("Not a module")










