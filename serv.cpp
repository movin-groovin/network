
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_server

#include "serv.hpp"



const char *masterPassw = "c54ccf7980cf302fb8eb68aa66c4f1b24b5d78ef";
ShpSigwait spSwt (new SIGWAIT_DATA);
ShpTskMap spTmap (new ShpTskMap);
FLAGS_DATA flgDat;



void* CallFunc (void * pvPtr) {
	return static_cast <CTask*> (pvPtr)->WorkerFunction ();
}


std::string StrError (int errCode) {
	std::vector <char> tmp (1024 + 1);
	
	return std::string (strerror_r (errCode, &tmp[0], 1024));
}


int main (int argc, char *argv []) {
	try {
		int ret;
		const char *chName = "network_server";
		struct sigaction sigAct;
		ShpTask hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
		pthread_t thrSigHnd;
		sigset_t sigMsk;
		
		
		if ((ret = daemonize (chName)) != 0) {
			return ret;
		}
		
		if (argc < 3) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Need the config path and special string\n");
#endif
			return 0;
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
			// read config
			//
			
			//
			// to create listen socket
			//
			
			
			while (true) {
				pthread_mutex_lock (&spSwt->syncMut);
				pthread_cond_wait (&spSwt->syncCond);
				if (flgDat.reread) {
					
					flgDat.reread = false;
					pthread_mutex_unlock (&spSwt->syncMut);
					
					//
					
					break;
				} else if (flgDat.terminate) {
					
					flgDat.terminate = false;
					pthread_mutex_unlock (&spSwt->syncMut);
#ifndef NDEBUG
					syslog (LOG_WARNING, "Normal termination of the server\n");
#endif
					
					//
					
					return 0;
				} else {
					// we mustn't come here
					pthread_mutex_unlock (&spSwt->syncMut);
				}
			}
			
			//
		}
	} catch (CException & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Has caught an exception at root function: %s\n", Exc.MsgError ().c_str ());
#endif
		return 100;
	} catch (std::exception & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Has caught an exception at root function: %s\n", Exc.MsgError ().c_str ());
#endif
		return 101;
	}
	
	
	return 0;
}


void* CHandlerTask::WorkerFunction () {
	int ret, sigRet;
	sigmask_t sigMsk;
	
	
	sigemptyset (&sigMsk);
	sigaddset (&sigMsk, SIGHUP);
	sigaddset (&sigMsk, SIGTERM);
	
	while (true) {
		if ((ret = sigwait (&sigMsk, &sigRet)) != 0) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of sigwait: %s, code: %d\n", StrError (ret).c_str (), ret);
			usleep (1000);
#endif
		}
		
		switch (sigRet) {
			case SIGHUP:
				
				pthread_exit (NULL);
				break;
			//
			case SIGSTOP:
				
				pthread_exit (NULL);
				break;
			//
			default:
#ifndef NDEBUG
				syslog (LOG_WARNING, "Hace caught unexpected signal\n");
#endif
				;
		}
		
		//
	}
	
	
	return NULL;
}


void* CNetworkTask::WorkerFunction () {
	return NULL;
}










