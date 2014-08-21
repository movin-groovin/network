#!/usr/bin/python
#-*- encoding: utf-8 -*-


import sys, os, socket, struct, binascii


g_len = 16


def main ():
	str1 = "Hello"
	dat = struct.pack ("i", len (str1) ** 3)
	dat = dat + struct.pack ("B", 0) * (g_len - len (dat))
	
	print (len (dat))
	print (binascii.hexlify (dat))
	print dat
	
	return
	


main ()
