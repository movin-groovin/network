
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_server

#include "serv.hpp"



int maxCnct = 10;
const char *masterPassw = "c54ccf7980cf302fb8eb68aa66c4f1b24b5d78ef";
const char *confPath = "/home/yarr/src/srv/network/network_server.conf"
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


void Sigusr2Action (int sigNum, siginfo_t *sigInf, void *pvCont) {
	// just to interrupt;
	return;
}


int GetConfigInfo (
	const std::string & strCnf,
	std::string & shadowConf, 
	std::unordered_map <std::string, std::string> & authData
)
{
	std::string shdCnf ("shadow_conf"), authStr ("user1"), tmpStr;
	
	
	while (std::getline (iFs, tmpStr)) {
		if (tmpStr[0] == '#' || tmpStr[0] == ' ') {
			tmpStr.clear ();
			continue;
		}
		
		if (tmpStr.compare (0, shdCnf.size (), shdCnf) == 0) {
			size_t firstPos, lastPos;
			
			if ((firstPos = tmpStr.find ('=')) == std::string::npos) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct shadow_conf string\n");
#endif
				return 1;
			}
			while (tmpStr[firstPos] == ' ') firstPos++;
			lastPos = tmpStr.find (' ', firstPos)
			if (lastPos == std::string::npos)
				lastPos = tmpStr.length () - 1;
			else
				--lastPos;
			shadowConf.assign (tmpStr, firstPos, lastPos - firstPos + 1);
#ifndef NDEBUG
			syslog (LOG_ERR, "Result sdhadow conf: %s\n", shadowConf.c_str ());
#endif
		} else if (tmpStr.compare (0, authStr.size (), authStr)) {
			size_t firstPos, lastPos, midPos;
			std::string strUsr, strPass;
			
			if ((firstPos = tmpStr.find ('=')) == std::string::npos ||
				(midPos = tmpStr.find (':')) == std::string::npos
				)
			{
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct auth string\n");
#endif
				return 2;
			}
			while (tmpStr[firstPos] == ' ') firstPos++;
			lastPos = tmpStr.find (' ', firstPos)
			if (lastPos == std::string::npos)
				lastPos = tmpStr.length () - 1;
			else
				--lastPos;
			tmpStr = std::string ().assign (tmpStr, firstPos, lastPos - firstPos);
			
			authData.insert (std::make_pair (tmpstr.substr (0, midPos - firstPos),
							 tmpStr.substr (midPos + 1, lastPos - midPos))
			);
		}
		
		tmpStr.clear ();
	}
	
	
	return 0;
}


int main (int argc, char *argv []) {
	try {
		int ret, sockLstn;
		const char *chName = "network_server";
		struct sigaction sigAct;
		ShpTask hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
		pthread_t thrSigHnd;
		sigset_t sigMsk;
		
		//
		// daemonize and logging
		//
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
		
		//
		// Adjust signals and mask
		//
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
			std::string shadowConf;
			std::unordered_map <std::string, std::string> authData;
			
			//
			// Sig hnd thread creating
			//
			if ((ret = hndTask->Start (&thrSigHnd, NULL, NULL)) != 0) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of pthread_create: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
			
			//
			// read config
			//
			std::ifstream iFs (confPath);
			if (!iFs) {
				ret = errno;
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of file opening: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
			
			if (0 != (ret = GetConfigInfo (iFs, shadowConf, authData))) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of GetConfigInfo, code: %d\n", ret);
#endif
				return 1;
			}
			iFs.close ();
			iFs.clear ();
			
			//
			// sig mask adjusting and SIGUSR2 behavior
			//
			sigfillset (&sigMsk);
			sigaddset (&sigMsk, SIGUSR2);
			if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
				ret = errno;
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
			sigAct.sa_sigaction = &Sigusr2Action;
			sigAct.sa_flags = SA_SIGINFO | SA_INTERRUPT;
			sigfillset (&sigAct.sa_mask);
			if ((ret = sigaction (SIGUSR2, &sigAct, NULL)) == -1) {
#ifndef NDEBUG
				ret = errno;
				syslog (LOG_ERR, "Error of sigaction: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
			
			//
			// to create listen socket and add to shadow config
			//
			if (-1 == (sockLstn = CreateListenSock (shadowConf))) {
				
				return 1;
			}
			
			while (true) {
				//
				// To accept connection and add to shadow config
				//
				int sock = AcceptConnection (shadowConf, authData);
				// -1 - error, -2 - have gotten SIGUSR2
				if (-1 == sock)
					return 2;
				else {
					//
					// To create new working thread
					//
					;
				}
				
				//
				// finish work
				//
				pthread_mutex_lock (&spSwt->syncMut);
				//pthread_cond_wait (&spSwt->syncCond);
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










