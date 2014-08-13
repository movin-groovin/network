
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_sever

#include "serv.hpp"



void* CallFunc (void * pvPtr) {
	return static_cast <CTask*> (pvPtr)->WorkerFunction ();
}


std::string StrError (int errCode) {
	std::vector <char> tmp (1024 + 1);
	
	return std::string (strerror_r (errCode, &tmp[0], 1024));
}


int main (int argc, char *argv []) {
	int ret;
	const char *chName = "network_server";
	struct sigaction sigAct;
	std::shared_ptr <CTask> hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
	pthread_t thrSigHnd;
	sigset_t sigMsk;
	
	
	if ((ret = daemonize (chName)) != 0) {
		return ret;
	}

#ifndef NDEBUG
	openlog (chName, LOG_PID, LOG_USER);
#endif
	
	sigAct.sa_handler = SIG_DFL;
	sigAct.sa_flags = 0;
	sigemptyset (&sigAct.sa_mask);
	if ((ret = sigaction (SIGHUP, &sigAct, NULL)) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of sigaction: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
		return ret;
	}
	
	sigemptyset (&sigMsk);
	if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
		return ret;
	}
	
	
	while (true) {
		if ((ret = hndTask->Start (&thrSigHnd, NULL, NULL)) != 0) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of pthread_create: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
			return ret;
		}
		
		
		
		//
	}
	
	
	return 0;
}


void* CHandlerTask::WorkerFunction () {
	return NULL;
}


void* CNetworkTask::WorkerFunction () {
	return NULL;
}










