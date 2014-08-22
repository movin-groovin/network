#!/usr/bin/python
#-*- encoding: utf-8 -*-


import sys, os, socket, struct, binascii


g_headerSize = 256



def SendString (s, strDat):
	data = struct.pack ("i", len (strDat))
	data = data + struct.pack ("B", 0) * (g_headerSize - len (data))
	data += strDat
	
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
	
	sys.stdout.write (ReadString (sock))
	#SendString (sock, raw_input ("Enter login: "))
	SendString (sock, sys.stdin.read ())
	sys.stdout.write (ReadString (sock))
	#SendString (sock, raw_input ("Enter password: "))
	SendString (sock, sys.stdin.read ())
	sys.stdout.write (ReadString (sock))
	
	while True:
		#SendString (sock, raw_input ("Command: "))
		SendString (sock, sys.stdin.read ())
		sys.stdout.write ("Results: ")
		print (ReadString (sock))
		
	print ("=====================")
	
	
	return
	


main ()















