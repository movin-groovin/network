
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_server

#include "serv.hpp"



const int maxCnct = 10, g_sleepAcceptTimeout = 10, g_sleepWorkerThread = 1; // in seconds
const char *masterPassw = "c54ccf7980cf302fb8eb68aa66c4f1b24b5d78ef";
const char *confPath = "/home/yarr/src/srv/network/network_server.conf"
const char *tcp4Data = "/proc/net/tcp";
const helloStr1 = "Enter a name: ";
const helloStr2 = "\nEnter a password: ";

ShpTskMap g_shpTmap (new CTaskMap);
FLAGS_DATA g_flgDat;



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
	std::string & portStr, 
	std::unordered_map <std::string, std::string> & authData
)
{
	std::string shdCnf ("shadow_conf"), authStr ("user1"), portStr ("port"), tmpStr;
	
	
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
		} else if (tmpStr.compare (0, portStr.size (), portStr) == 0) {
			size_t strInd = tmpStr.find ('=');
			if (strInd == std::string::npos) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct port string, line: %d\n", __LINE__);
#endif
				return 3;
			}
			
			while (tmpStr[strInd] == ' ') ++strInd;
			if (strInd == tmpStr.length ()) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct port string, line: %d\n", __LINE__);
#endif
				return 4;
			}
			
			portStr.assign (tmpStr, strInd, strInd.length () - strInd + 1);
		}
		
		tmpStr.clear ();
	}
	
	
	return 0;
}


int AddToShadowConfig (int socket, const std::string & shadowConf) {
	return 1;
}


int CreateListenSock (const std::string & shadowConf, int portNum, bool nonBlck = true) {
	int ret, sock, fdVal;
	struct sockaddr sockAddr;
	
	
	assert (shadowConf.length () != 0);
	
	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of socket: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	if (nonBlck) {
		if (-1 == (fdVal = fcntl (sock, F_GETFL, 0))) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of first fcntl: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		
		if (-1 == fcntl (sock, F_SETFL, fdVal | O_NONBLOCK)) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of second fcntl: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
	}
	
	memset (&sockAddr, 0, sizeof sozkAddr);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	while (true) {
		sockAddr.sin_port = htons (portNum);
		if (-1 == bind () && errno == EADDRINUSE) {
			++portNum;
			continue;
		} else {
		ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of bind: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		
		break;
	}
	
	if (-1 == listen (socket, maxCnct / 2)) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of listen: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	AddToShadowConfig (socket, shadowConf, true);
	
	
	return sock;
}


ssize_t WriteToSock (int sock, const void *buf, size_t num, int flags) {
	unsigned char *chPtr = static_cast <unsigned char*> (buf);
	ssize_t total = 0, cur;
	int ret;
	
	while (cur = send (sock, chPtr + total, num - total, flags)) {
		if (cur == -1 && errno == EINTR) continue;
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of send: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return ret;
		}
		
		total += cur;
	}
	
	return total;
}


ssize_t ReadFromSock (int sock, void *buf, size_t num, int flags) {
	unsigned char *chPtr = static_cast <unsigned char*> (buf);
	ssize_t total = 0, cur;
	int ret;
	
	while (cur = recv (sock, chPtr + total, num - total, flags)) {
		if (cur == -1 && errno == EINTR) continue;
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of recv: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		
		total += cur;
	}
	
	return total;
}


int AcceptConnection (
	int lstnSock
	const std::string & shadowConf,
	std::unordered_map <std::string, std::string> & authData
)
// -1 - error, -2 - have gotten SIGUSR2 or have elapsed timeout, more or equal 0 - successful result
{
	int sock, ret;
	ssize_t retLen;
	std::vector <char> datBuf (1024);
	std::string strPass;
	
	
	if ((sock = accept (lstnSock, NULL, NULL)) == -1 && errno == EINTR) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Before completion of accept have gotten SIGUSR1 signal\n");
#endif
		return -2;
	} else if (sock == -1 && errno != EWOULDBLOCK) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of accept: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		return -1;
	} else if (sock == -1 && errno == EWOULDBLOCK) {
		struct pollfd polDat = {lstnSock, POLLIN, 0};
		if ((ret = poll (&polDat, 1, g_sleepAcceptTimeout * 1000) == -1 && && errno == EINTR)) {
#ifndef NDEBUG
			syslog (LOG_WARNING, "Before completion of poll have gotten SIGUSR1 signal\n");
#endif
			return -2;
		} else if (ret == -1) { // errno != EINTR
		ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of poll: %s, errno: %d, in %s at file: %s, on line: %s\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		if (ret == 0) {
#ifndef NDEBUG
			syslog (LOG_WARNING, "Have elapsed time at call of poll\n");
#endif
			return -2;
		}
	}
	
	AddToShadowConfig (sock, shadowConf, false);
	
	//
	// Check auth info
	//
	for (auto &entry : authData) {
		if (strlen (helloStr1) != WriteToSock (sock, helloStr1, strlen (helloStr1), 0)) return -1;
		if (-1 == (retLen = ReadFromSock (sock, &datBuf[0], datBuf.size () - 1, 0))) return -1;
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten user name: %s\n", &datBuf[0]);
#endif	
		
		
		auto it = authData.find (&datBuf[0]);
		if (it == authData.end ()) {
			return -1;
		}
		
		if (strlen (helloStr1) != WriteToSock (sock, helloStr2, strlen (helloStr1), 0)) return -1;
		if (-1 == (retLen = ReadFromSock (sock, &datBuf[0], datBuf.size () - 1, 0))) return -1;
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten password: %s\n", &datBuf[0]);
#endif	
		
		if (it->second != &datBuf[0]) {
			return -1;
		}
		return sock;
	}
	
	
	return -1;
}


int main (int argc, char *argv []) {
	try {
		int ret, sockLstn;
		const char *chName = "network_server";
		struct sigaction sigAct;
		ShpTask hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
		pthread_t thrSigHnd;
		
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
		// Adjust SIGHUP behavior to default
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
		
		//
		// Main loop
		//
		while (true) {
			sigset_t sigMsk;
			std::string shadowConf, portNumStr;
			std::unordered_map <std::string, std::string> authData;
			
			//
			// mask of signals adjusting
			//
			sigfillset (&sigMsk);
			if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
				ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
		
			//
			// Signal-handler thread creating
			//
			if ((ret = hndTask->Start (&thrSigHnd, NULL)) != 0) {
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
			
			if (0 != (ret = GetConfigInfo (iFs, shadowConf, portNumStr, authData))) {
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
			sigdelset (&sigMsk, SIGUSR1);
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
			if ((ret = sigaction (SIGUSR1, &sigAct, NULL)) == -1) {
#ifndef NDEBUG
				ret = errno;
				syslog (LOG_ERR, "Error of sigaction: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
				return ret;
			}
			
			//
			// to create listen socket and add to shadow config
			//
			int portNum;
			std::istringstream issCnv;
			issCnv.str (portNumStr)
			issCnv >> portNum;
			if (-1 == (sockLstn = CreateListenSock (shadowConf, portNum))) return 1;
			
			//
			// Loop of polling and accept
			//
			while (true) {
				//
				// To accept connection and add to shadow config
				//
				int sock = AcceptConnection (sockLstn, shadowConf, authData);
				// -1 - error, -2 - have gotten SIGUSR2 or have elapsed timeout, 0 or more - successful result
				if (sock >= 0) {
					pthread_t pthId;
					
					ShpTask netTask (new CNetworkTask (ShpParams (new TASK_PARAMETERS (sock))));
					if (!(ret = netTask.Start (&pthId, NULL))) {
#ifndef NDEBUG
						syslog (LOG_ERR, "Error of pthread_create: %s, code: %d; file: %s - line: %d\n",
								StrError (ret).c_str (), ret, __FILE__, __LINE__
						);
#endif
						return ret;
					}
					int tmp;
					pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, &tmp);
					pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &tmp);
					g_shpTmap.insert (pthId, netTask);
				}
				
				//
				// examine variables
				//
				if (g_flgDat.CheckReread ()) {
					g_flgDat.SetReread (false);
#ifndef NDEBUG
					syslog (LOG_WARNING, "Rereading of config file, and restarting\n");
#endif
					pthread_kill (thrSigHnd, SIGTERM);
					for (auto & entry : g_shpTmap)
						pthread_cancel (entry.first);
					g_shpTmap.clear ();
					sleep (1);
					
					break;
				} else if (g_flgDat.CheckTerm ()) {
					g_flgDat.SetTerm (false);
#ifndef NDEBUG
					syslog (LOG_WARNING, "Normal termination of the server\n");
#endif
					pthread_kill (thrSigHnd, SIGTERM);
					for (auto & entry : g_shpTmap)
						pthread_cancel (entry.first);
					g_shpTmap.clear ();
					sleep (1);
					
					return 0;
				} else {
					// we mustn't come here
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
			usleep (1000 * 100);
#endif
		}
		
		switch (sigRet) {
			case SIGHUP:
				g_flgDat.SetReread (true);
				pthread_exit (NULL);
				break;
			//
			case SIGTERM:
				g_flgDat.SetTerm (true);
				pthread_exit (NULL);
				break;
			//
			default:
#ifndef NDEBUG
				syslog (LOG_WARNING, "Hace caught unexpected signal\n");
#endif
				break;
		}
	}
	
	
	return NULL;
}


void* CNetworkTask::WorkerFunction () {
	return NULL;
}










