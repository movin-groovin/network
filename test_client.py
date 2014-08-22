#!/usr/bin/python
#-*- encoding: utf-8 -*-


import sys, os, socket, struct, binascii


g_headerSize = 256



def SendString (s, strDat):
	data = struct.pack ("i", len (strDat))
	data = data + struct.pack ("B", 0) * (g_headerSize - len (data))
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
	#print (type (totLen))
	#print (totLen)
	
	data = ""
	while totLen > 0:
		ret = s.recv (totLen)
		totLen -= len (ret)
		data += ret
		
	return data



def main ():
	if len (sys.argv) < 3:
		print ("Need host and port parameters")
		return
	
	if __debug__:
		print ("Connecting to: {0}, at port: {1}".format (sys.argv[1], sys.argv[2]))
	
	sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
	sock.connect ((sys.argv[1], int (sys.argv[2])))
	
	# first string from server
	sys.stdout.write (ReadString (sock))
	# login name
	sndStr = sys.stdin.readline ()
	SendString (sock, sndStr[0 : len (sndStr) - 1])
	sys.stdout.write (ReadString (sock))
	# password
	sndStr = sys.stdin.readline ()
	SendString (sock, sndStr[0 : len (sndStr) - 1])
	sys.stdout.write (ReadString (sock))
	
	while True:
		# to send a command
		sndStr = sys.stdin.readline ()
		SendString (sock, sndStr[0 : len (sndStr) - 1])
		sys.stdout.write ("Results: ")
		print (ReadString (sock))
		
	print ("=====================")
	#http://pymotw.com/2/struct/
	
	
	return
	


main ()















