#!/usr/bin/python
#-*- encoding:utf-8 -*-


import sys, os, time



def main ():
	pid = os.fork ()
	
	if pid == -1:
		print "Error at creation of child"
		return 0
	
	if pid > 0:
		print "The child has created"
		return 0
	else:
		while True:
			print "======================= Inside infinite loop ======================="
			time.sleep (3)
	
	return 0
	
	
	

if __name__ == "__main__":
	main ()
else:
	raise RuntimeError ("Can't run like a module")
