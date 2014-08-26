
#include <iostream>
#include <cstdio>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>



int main (int argc, char *argv []) {
	int ret;
	std::string strBuf ("===== qwerty =====\n");
	
	ret = write (1, strBuf.c_str (), strBuf.size ());
	syslog (LOG_NOTICE, "============== VALUE: %d ==============\n", ret);
	syslog (LOG_NOTICE, "INTPUT: %d; OUTPUT:%d; ERROUT: %d\n", close (0), close (1), close (2));
	
	return 0;
}



