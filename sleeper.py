#!/usr/bin/python
#-*- encoding:utf-8 -*-


import sys, os, time



def main ():
	
	while True:
		print "======================= Inside infinite loop ======================="
		time.sleep (3)
	
	return 0
	
	
	

if __name__ == "__main__":
	main ()
else:
	raise RuntimeError ("Can't run like a module")
