#!/usr/bin/python
#-*- encoding: utf-8 -*-


import sys, os, socket, struct, binascii


g_headerSize = 256
g_MaxDataLen = 128 * 1024 * 1024
g_intLen = 4
g_waitSecs = 30
g_waitMilSecs = 0

ExecuteByChild = 0x0
ExecuteThemself = 0x1
ServerAnswer = 0x2
ServerRequest = 0x4
ClientAnswer = 0x8

Positive = 0
Negative = 1

BadName = 0x0
BadPass = 0x1
GetName = 0x2
GetPass = 0x4,
WaitCommand = 0x8
InternalServerError = 0x10
TooLong = 0x16



def SendString (s, strDat, cmdType = 0, retStatus = 0, infStatus = 0):
	if len (strDat) > g_MaxDataLen:
		if __debug__: raise Exception ("Too long data for transfering")
	#print ("WRITE: ", strDat, cmdType, retStatus, infStatus)
	
	data = struct.pack ("i", len (strDat))
	data += struct.pack ("i", cmdType)
	data += struct.pack ("i", retStatus)
	data += struct.pack ("i", infStatus)
	data += struct.pack ("B", 0) * (g_headerSize - len (data))
	data += strDat
	#print binascii.hexlify (data)
	
	totLen = 0
	while totLen < len (data):
		ret = s.send (data[totLen : len (data)])
		totLen += ret
		
	return totLen


def ReadString (s):
	data = ""
	totLen = 0
	
	while True:
		ret = s.recv (g_headerSize - totLen)
		totLen += len (ret)
		data += ret
		if totLen == g_headerSize: break
	#print binascii.hexlify (data)
	
	totLen = struct.unpack ("i", data[0:4])[0]
	cmdType = struct.unpack ("i", data[4:8])[0]
	retStatus = struct.unpack ("i", data[8:12])[0]
	infStatus = struct.unpack ("i", data[12:16])[0]
	#print ("READ: ", totLen, cmdType, retStatus, infStatus)
	#print (type (totLen))
	#print (totLen)
	if totLen > g_MaxDataLen:
		SendString (s, "Too long data for transfering", ClientAnswer, Negative, TooLong)
		if __debug__: raise Exception ("Too long data for acceptance")
		else: return -1, ServerAnswer, Negative, TooLong, ""
	
	data = ""
	while totLen > 0:
		ret = s.recv (totLen)
		totLen -= len (ret)
		data += ret
		
	return totLen, cmdType, retStatus, infStatus, data



def main ():
	if len (sys.argv) < 3:
		print ("Need host and port parameters")
		return
	
	if __debug__:
		print ("Connecting to: {0}, at port: {1}".format (sys.argv[1], sys.argv[2]))
	
	sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
	timeVal = struct.pack ("LL", g_waitSecs, 1000 * g_waitMilSecs)
	sock.setsockopt (socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeVal)
	sock.setsockopt (socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeVal)
	sock.connect ((sys.argv[1], int (sys.argv[2])))
	
	# first string from server
	ret = ReadString (sock)
	sys.stdout.write (ret[4])
	
	# login name
	sndStr = sys.stdin.readline ()
	SendString (sock, sndStr[0 : len (sndStr) - 1])
	ret = ReadString (sock)
	if ret[2] == Negative and ret[3] == BadName:
		print ("Incorrect login")
		return 1
	sys.stdout.write (ret[4])
	
	# password
	sndStr = sys.stdin.readline ()
	SendString (sock, sndStr[0 : len (sndStr) - 1])
	ret = ReadString (sock)
	if ret[2] == Negative and ret[3] == BadPass:
		print ("Incorrect password")
		return 2
	sys.stdout.write (ret[4])
	
	while True:
		# to send a command
		sndStr = sys.stdin.readline ()
		if sndStr[0:len (sndStr) - 1] == "exit":
			SendString (sock, sndStr[0 : len (sndStr) - 1])
			sys.stdout.write ("Results: {strr}".format (strr = ReadString (sock)[4]))
			return
			
		SendString (sock, sndStr[0 : len (sndStr) - 1])
		#sys.stdout.write ("Results: ")
		ret = ReadString (sock)
		if ret[1] == ServerAnswer and ret[3] & InternalServerError and (not ret[3] & WaitCommand):
			print ("Have got an error from server, we can't continue command execution")
			sys.stdout.write ("Results: {strr}".format (strr = ret[4]))
			return 3
		sys.stdout.write ("Results: {strr}".format (strr = ret[4]))
		
	print ("===============================================================")
	#http://pymotw.com/2/struct/
	
	
	return
	

#try:
main ()
"""
except Exception as Exc:
	print ("Have caught an exception")
	print Exc
	exit (10001)
"""













