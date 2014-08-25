
// g++ -lstd=c++11 -o 1_elf 1.cpp

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstring>
#include <cerrno>
#include <cstdio>

/*
void func1 (int);


class COne {
public:
	//void func1 ();
	friend void func1 (int a) {std::cout << "Friend function\n";}
	
public:
	void f () {func1 (1);}
	
};
*/


std::string StrError (int errCode) {
	std::vector <char> tmp (1024 + 1);
	
	return std::string (strerror_r (errCode, &tmp[0], 1024));
}


int ReadFromDescriptor (int rdFd, std::vector <char> & outStr, int waitMilSec) {
	ssize_t totLen = 0, curLen;
	int ret, flVal, timeWaitMilSec = 100;
	
	//
	// File object flags adjusting
	//
	if ((flVal = fcntl (rdFd, F_GETFL, 0)) == -1) {
#ifndef NDEBUG
		ret = errno;
		printf ("Error of fcntl (get file flags): %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	if ((flVal = fcntl (rdFd, F_SETFL, flVal | O_NONBLOCK)) == -1) {
#ifndef NDEBUG
		ret = errno;
		printf ("Error of fcntl (set file flags): %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	//
	// Reading operation
	//
	while ((curLen = read (rdFd, &outStr[0] + totLen, outStr.size () - 1 - totLen)) != 0) {
		if (curLen == -1 && errno == EAGAIN) { // wait
			usleep (1000 * timeWaitMilSec);
			timeWaitMilSec *= 2;
			if (timeWaitMilSec > waitMilSec) return -1;
			continue;
		}
		if (curLen == -1 && errno == EINTR) return -2; // SIGUSR2 interrupts us
		if (curLen == -1) {
#ifndef NDEBUG
			ret = errno;
			printf ("Error of read: %s, code: %d, at file: %s, at line: %d\n",
					StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		
		totLen += curLen;
		if (totLen == outStr.size () - 1) outStr.resize (2 * outStr.size ());
		timeWaitMilSec = 100;
	}
	outStr[totLen] = '\0';
	
	
	return totLen;
}


int Pipe (const std::vector <std::string> & cmdStr, std::vector <char> & outStr, int & retValue) {
	int ret, pid, pipeFd[2];
	struct sigaction sigAct;
	sigset_t sigMask;
	
	
	if (-1 == pipe (pipeFd)) {
#ifndef NDEBUG
		ret = errno;
		printf ("Error of pipe: %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	if ((pid = fork ()) < 0) // error
	{
#ifndef NDEBUG
		ret = errno;
		printf ("Error of fork: %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		close (pipeFd[0]);
		close (pipeFd[1]);
		return -1;
	} else if (pid > 0) // parent
	{
		int retStatus, waitMilSec = 100;
		
		
		close (pipeFd[1]);
		if (0 > ReadFromDescriptor (pipeFd[0], outStr, 10 * 1000)) {
			kill (pid, SIGKILL);
			close (pipeFd[0]);
			return -1;
		}
		close (pipeFd[0]);
		
		while (waitMilSec < 10 * 1000 / 10)
		{
			if (-1 == (ret = waitpid (pid, &retStatus, WNOHANG))) {
#ifndef NDEBUG
				ret = errno;
				printf ("Error of waitpid: %s, code: %d, at file: %s, at line: %d\n",
						StrError (ret).c_str (), ret, __FILE__, __LINE__
				);
#endif
				kill (pid, SIGKILL);
				return -1;
			} else if (ret == 0) {
				usleep (1000 * waitMilSec);
				waitMilSec *= 2;
			} else {
				retValue = WEXITSTATUS (retStatus);
				return 0;
			}
		}
		
#ifndef NDEBUG
		printf ("Time at waitpid have elapsed, file: %s, line: %d\n", __FILE__, __LINE__);
#endif
		kill (pid, SIGKILL);
		return -1;
	} else // (pid == 0) // child
	{
		close (pipeFd[0]);
		sigemptyset (&sigMask);
		
		if (sigprocmask (SIG_SETMASK, &sigMask, NULL) == -1) {
#ifndef NDEBUG
			ret = errno;
			printf ("Error of sigprocmask at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		close (0); // stdin
		if (-1 == dup2 (pipeFd[1], 1) || -1 == dup2 (pipeFd[1], 2)) { // stdout and stderr
#ifndef NDEBUG
			ret = errno;
			printf ("Error of dup2 at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		char *parArr [15 + 1];
		for (int i = 0; i < cmdStr.size (); ++i) parArr [i] = const_cast <char*> (cmdStr[i].c_str ());
		parArr[cmdStr.size ()] = NULL; 
		
		if (-1 == execvp (cmdStr [0].c_str (), parArr)) {
#ifndef NDEBUG
			ret = errno;
			printf ("Error of execlp at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
	}
	
	
	return -2;
}


int main (int argc, char * argv[]) {
	/*
	COne obj;
	
	
	obj.f ();
	std::string str1 ("12345");
	std::istringstream issCnv;
	
	issCnv.str (str1);
	int var;
	issCnv >> var;
	
	std::cout << "Var: " << var << std::endl;
	* */
	if (argc < 2) {
		std::cout << "Enter a command\n";
		return 1;
	}
	
	int ret, status;
	std::vector <char> outStr (4096);
	std::vector <std::string> cmdStr;
	
	
	for (int i = 0; i < argc - 1; ++i) cmdStr.push_back (argv[1 + i]);
	ret =  Pipe (cmdStr, outStr, status);
	std::cout << "Return: " << ret << std::endl;
	std::cout << "Status: " << status << std::endl;
	std::cout << "String: " << &outStr[0] << std::endl;
	
	
	return 0;
}





















