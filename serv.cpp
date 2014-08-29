
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp -g ../mon_daemon/daemonize.cpp -o network_server
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_server
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp -DNDEBUG ../mon_daemon/daemonize.cpp -o network_server

#include "serv.hpp"



const char *g_masterPassw = "c54ccf798";
const char *g_masterName = "master";
const char *g_tcp4Data = "/proc/net/tcp";
const char *g_helloStr1 = "Enter a name: ";
const char *g_helloStr2 = "Enter a password: ";
const char *g_shadowCnf = "/etc/1234DEADBEAF4321/tcp4.txt";
const char *g_defPort = "12345";
const char *g_tmpPidFile = "/tmp/network_ef7cdf537.pid";

const int g_maxCnct = 10, g_sleepAcceptTimeout = 10, g_sleepWorkerThread = 1; // at seconds
const int g_maxParLen = 150;
const int g_totalWaitPid = 128 * 100; // at milliseconds, 12.8 sec > 10 sec, values at milliseconds
const int g_waitStartChildProcMilSec = 100;
const int g_ordSockTimeoutSec = 600; // 10min

ShpTskMap g_shpTmap (new CTaskMap);
FLAGS_DATA g_flgDat;
const char *g_confPath, *g_cmdLineSpecStr;



void Sigusr12Action (int sigNum, siginfo_t *sigInf, void *pvCont) {
	// just to interrupt;
	return;
}


int GetConfigInfo (
	std::ifstream & iFs,
	std::string & shadowConf,
	std::string & portStr, 
	std::unordered_map <std::string, std::string> & authData
)
{
	std::string shdStrCnf ("shadow_conf"), authStrCnf ("user"), portStrCnf ("port");
	std::string tmpStr;
	
	
	while (std::getline (iFs, tmpStr)) {
		if (tmpStr[0] == '#' || tmpStr[0] == ' ' || tmpStr.size () == 0) {
			tmpStr.clear ();
			continue;
		}
#ifndef NDEBUG
		syslog (LOG_ERR, "String: %s\n", tmpStr.c_str ());
#endif
		
		if (tmpStr.compare (0, shdStrCnf.size (), shdStrCnf) == 0) {
			size_t firstPos, lastPos;
			
			if ((firstPos = tmpStr.find ('=')) == std::string::npos) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct shadow_conf string\n");
#endif
				return 1;
			}
			++firstPos;
			while (tmpStr[firstPos] == ' ') firstPos++;
			lastPos = tmpStr.find (' ', firstPos);
			if (lastPos == std::string::npos)
				lastPos = tmpStr.length () - 1;
			else
				--lastPos;
			shadowConf.assign (tmpStr, firstPos, lastPos - firstPos + 1);

		} else if (tmpStr.compare (0, authStrCnf.size (), authStrCnf) == 0) {
			size_t firstPos, lastPos, midPos;
			std::string strUsr, strPass;	
			firstPos = tmpStr.find ('=');
			midPos = tmpStr.find (':');
			if (firstPos == std::string::npos || midPos == std::string::npos)
			{
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct auth string\n");
#endif
				return 2;
			}
			++firstPos;
			while (tmpStr[firstPos] == ' ') firstPos++;
			lastPos = tmpStr.find (' ', firstPos);
			if (lastPos == std::string::npos)
				lastPos = tmpStr.length () - 1;
			else
				--lastPos;
			authData.insert (std::make_pair (tmpStr.substr (firstPos, midPos - firstPos),
											 tmpStr.substr (midPos + 1, lastPos - midPos)
							)
			);
		} else if (tmpStr.compare (0, portStrCnf.size (), portStrCnf) == 0) {
			size_t strInd = tmpStr.find ('=');
			if (strInd == std::string::npos) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct port string, line: %d\n", __LINE__);
#endif
				return 3;
			}
			++strInd;
			while (tmpStr[strInd] == ' ') ++strInd;
			if (strInd == tmpStr.length ()) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Not correct port string, line: %d\n", __LINE__);
#endif
				return 4;
			}
			
			portStr.assign (tmpStr, strInd, tmpStr.length () - strInd + 1);
#ifndef NDEBUG
			syslog (LOG_ERR, "Result port str: %s\n", portStr.c_str ());
#endif
		}
		
		tmpStr.clear ();
	}
	
	// master name and passw
	authData.insert (std::make_pair (g_masterName, g_masterPassw));
	
	
	return 0;
}


int AddToShadowConfig (int socket, const std::string & shadowConf, bool isListen) {
	return 1;
}


int CreateListenSock (const std::string & shadowConf, int portNum, bool nonBlck = true) {
	int ret, sock, fdVal;
	struct sockaddr_in sockAddr;
	
	
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
	
	memset (&sockAddr, 0, sizeof sockAddr);
	sockAddr.sin_family = AF_INET;
//#ifndef NDEBUG
//	assert (0 != inet_aton ("127.0.0.1", &sockAddr.sin_addr));
//#else
	sockAddr.sin_addr.s_addr = htonl (INADDR_ANY);
//#endif
	while (true) {
//syslog (LOG_WARNING, "PORT: %d\n", portNum);
		sockAddr.sin_port = htons (portNum);
		if (-1 == (ret = bind (sock, reinterpret_cast <sockaddr*> (&sockAddr), sizeof sockAddr)) && errno == EADDRINUSE) {
			++portNum;
			continue;
		} else if (ret == -1) {
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
	
	if (-1 == listen (sock, g_maxCnct / 2)) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of listen: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	AddToShadowConfig (sock, shadowConf, true);
	
	
	return sock;
}

//
// -1 - error, -2 - was interrapted by SIGUSR2, more 0 - good result
//
ssize_t WriteToSock (int sock, const char *buf, int flags, const DATA_HEADER & hdrInf) {
	ssize_t total, cur;
	size_t num = hdrInf.u.dat.dataLen;
	int ret;
	
	
	assert (hdrInf.u.dat.dataLen <= DATA_HEADER::MaxDataLen);
	//
	// header transfer
	//
	total = 0;
	while (total < sizeof hdrInf) {
		cur = send (sock, reinterpret_cast <const char*> (&hdrInf) + total, sizeof hdrInf - total, flags);
		if (cur == -1 && errno == EINTR) {
			// have gotten SIGUSR2, to terminate transferring
#ifndef NDEBUG
			syslog (LOG_ERR, "Transfer was interrupted by SIGUSR2\n");
#endif
			return -2;
		}
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of send: %s, errno: %d, in: %s, at file: %s, on line: %d\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		if (cur == 0) break; // EOF
		
		total += cur;
	}
	
	//
	// useful data transfer
	//
	if (num == 0) return 0;
	total = 0;
	while (total < num) {
		cur = send (sock, static_cast <const char*> (buf) + total, num - total, flags);
		if (cur == -1 && errno == EINTR) {
			// have gotten SIGUSR2, to terminate transferring
#ifndef NDEBUG
			syslog (LOG_ERR, "Transfer was interrupted by SIGUSR2\n");
#endif
			return -2;
		}
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of send: %s, errno: %d, in %s, at file: %s, on line: %d\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		if (cur == 0) break; // EOF
		
		total += cur;
	}
	
	
	return total;
}

//
// -1 - error, -2 - was interrapted by SIGUSR2, -3 - incorrect data length, 0 or more - good result
//
ssize_t ReadFromSock (int sock, std::vector <char> & buf, int flags, DATA_HEADER & hdrInf) {
	ssize_t total, cur;
	int ret;
	
	
	//
	// header transfer
	//
	total = 0;
	while (total < sizeof hdrInf) {
		cur = recv (sock, reinterpret_cast <char*> (&hdrInf) + total, sizeof hdrInf - total, flags);
		if (cur == -1 && errno == EINTR) {
			// have gotten SIGUSR2, to terminate transferring
#ifndef NDEBUG
			syslog (LOG_ERR, "Transfer was interrupted by SIGUSR2\n");
#endif
			return -2;
		}
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of recv: %s, errno: %d, in %s, at file: %s, on line: %d\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		if (cur == 0) break; // EOF
		
		total += cur;
	}
	
	//
	// useful data transfer
	//
	total = 0;
	size_t num = hdrInf.u.dat.dataLen;
	if (num < 0 || num > DATA_HEADER::MaxDataLen) return -3;
	if (num == 0) return 0;
	
	if (num >= buf.size ()) buf.resize (num + 1);
	
	while (total < num) {
		cur = recv (sock, static_cast <char*> (&buf[0]) + total, num - total, flags);
		if (cur == -1 && errno == EINTR) {
			// have gotten SIGUSR2, to terminate transferring
#ifndef NDEBUG
			syslog (LOG_ERR, "Transfer was interrupted by SIGUSR2\n");
#endif
			return -2;
		}
		if (cur == -1 && errno != EINTR) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of recv: %s, errno: %d, in %s, at file: %s, on line: %d\n",
					StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
			);
#endif
			return -1;
		}
		if (cur == 0) break; // EOF
		
		total += cur;
	}
	
	
	return total;
}


int AcceptConnection (
	int lstnSock,
	const std::string & shadowConf
)
// -1 - error, -2 - have gotten SIGUSR2 or have elapsed timeout, more or equal 0 - successful result
{
	int sock, ret;
	
	
	assert (shadowConf.size () != 0);
	
	//
	// waiting of interested events
	//
	struct pollfd polDat = {lstnSock, POLLIN, 0};
	if ((ret = poll (&polDat, 1, g_sleepAcceptTimeout * 1000)) == -1 && errno == EINTR) {
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
	
	//
	// acceptance of client socket
	//
	if ((sock = accept (lstnSock, NULL, NULL)) == -1 && errno == EINTR) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Before completion of accept have gotten SIGUSR1 signal\n");
#endif
		return -2;
	} else if (sock == -1) { // errno == EWOULDBLOCK or != EWOULDBLOCK - no matter the fact is that our ready socket have evaporated
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of accept: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	//
	// setup timeouts for sockets
	//
	struct timeval timData = {g_ordSockTimeoutSec, 0};
	if (-1 == setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timData, sizeof timData)) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of setsockopt 1: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		close (sock);
		return -3;
	}
	if (-1 == setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &timData, sizeof timData)) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of setsockopt 1: %s, errno: %d, in %s at file: %s, on line: %s\n",
				StrError (ret).c_str (), ret, __PRETTY_FUNCTION__, __FILE__, __LINE__
		);
#endif
		close (sock);
		return -3;
	}
	
	AddToShadowConfig (sock, shadowConf, false);
	
	
	return sock;
}


int SignalsAdjusting () {
	struct sigaction sigAct;
	sigset_t sigMsk;
	int ret;
	
	
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
	// sig mask adjusting
	//
	sigfillset (&sigMsk);
	sigdelset (&sigMsk, SIGUSR1);
	if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
		return ret;
	}
	
	//
	// Adjust of SIGUSR1 and SIGUSR2 behavior
	//
	sigAct.sa_sigaction = &Sigusr12Action;
	sigAct.sa_flags = SA_SIGINFO | SA_INTERRUPT;
	sigfillset (&sigAct.sa_mask);
	if ((ret = sigaction (SIGUSR1, &sigAct, NULL)) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of sigaction: %s, code: %d, in file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return ret;
	}
	if ((ret = sigaction (SIGUSR2, &sigAct, NULL)) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of sigaction: %s, code: %d, in file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return ret;
	}
	
	
	return 0;
}


int AdjustConfig (
	std::string & shadowConf,
	std::string & portStr, 
	std::unordered_map <std::string, std::string> & authData
)
{
	int ret;
	
	if (g_confPath != NULL) {
		std::ifstream iFs (g_confPath);
		if (!iFs) {
			ret = errno;
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of file opening: %s, code: %d, file: %s\n",
					StrError (ret).c_str (), ret, g_confPath
			);
#endif
			return ret;
		}
		
		if (0 != (ret = GetConfigInfo (iFs, shadowConf, portStr, authData))) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of GetConfigInfo, code: %d\n", ret);
#endif
			return 1;
		}
		iFs.close ();
		iFs.clear ();
	} else {
		shadowConf = g_shadowCnf;
		portStr = g_defPort;
		authData.insert (std::make_pair (g_masterName, g_masterPassw));
	}
	
	return 0;
}


int main (int argc, char *argv []) {
	try {
		int ret, sockLstn;
		const char *chName = "network_server";
		ShpTask hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
		pthread_t thrSigHnd;
		
		
		// daemonize and logging
		if ((ret = daemonize (chName)) != 0) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of daemonize, ret code: %d\n", ret);
#endif
			return ret;
		}

		// cmdline checks
		if (argc < 2) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Need special string, for process identification, as a cmd-line parameter\n");
#endif
			return 0;
		}
		if (argc < 3) {
#ifndef NDEBUG
			syslog (LOG_WARNING, "A config hasn't provided\n");
#endif
		}
		
		// cmdline storing
		if (argc >= 3) {
			g_confPath = argv[1];
			g_cmdLineSpecStr = argv[2];
		} else {
			g_confPath = NULL;
			g_cmdLineSpecStr = argv[1]; 
		}
		
		// double starting check
		if (-1 == (ret = makeTmpPidFile (g_tmpPidFile))) {
#ifndef NDEBUG
			syslog (LOG_WARNING, "Error at makeTmpPidFile\n");
#endif
			return 1;
		} else if (ret) {
			if (1 == (ret = IsAlreadyRunning (g_cmdLineSpecStr, g_tmpPidFile))) {
#ifndef NDEBUG
				syslog (LOG_WARNING, "One instance of this process has started\n");
#endif
				return 0;
			} else if (ret == -1) {
#ifndef NDEBUG
				syslog (LOG_WARNING, "While execution IsAlreadyRunning have happened an error\n");
#endif
				std::ofstream oFs (g_tmpPidFile);
				std::ostringstream ossCnv;
				pid_t procPid = getpid ();
				
				if (!oFs) return 2;
				ossCnv << procPid;
				oFs << ossCnv.str ();
			}
		}

		// log adjusting
#ifndef NDEBUG
		openlog (chName, LOG_PID, LOG_USER);
#endif
		
		//
		// Adjust SIGHUP behavior to default, after change at daemonize
		//
		if (0 != (ret = SignalsAdjusting ())) return ret;
		
		//
		// Main loop
		//
		while (true) {
			sigset_t sigMsk;
			std::string shadowConf, portNumStr;
			std::unordered_map <std::string, std::string> authData;
		
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
			if (0 != (ret = AdjustConfig (shadowConf, portNumStr, authData))) return ret;
			
			//
			// to create listen socket and add to shadow config
			//
			int portNum;
			std::istringstream issCnv;
			issCnv.str (portNumStr);
			issCnv >> portNum;
			if (-1 == (sockLstn = CreateListenSock (shadowConf, portNum))) return 1;
			
			//
			// Loop of polling and accept
			//
			while (true) {
				std::string userName;
				int sock;
				//
				// To accept connection and add to shadow config
				//
				// -1 - error, -2 - have gotten SIGUSR2 or have elapsed timeout, 0 or more - successful result
				// -3 - errro of setsockopt
				while (-1 == (sock = AcceptConnection (sockLstn, shadowConf))) {
#ifndef NDEBUG
					syslog (LOG_WARNING, "AcceptConnection has returned -1\n");
#endif
					sleep (5);
					close (sockLstn);
					sockLstn = CreateListenSock (shadowConf, portNum);
				}
				
				//
				// To check g_shpTmap for died threads and if those will be find, to delete then from 
				//
				std::vector <pthread_t> thrArr;
				g_shpTmap->IterateEntries (
					[&] (std::pair <const pthread_t, std::shared_ptr <CTask>> & entry)->void 
					{
						if (pthread_kill (entry.first, 0) != 0) thrArr.push_back (entry.first);
					}
				);
				for (auto & entry : thrArr) g_shpTmap->Delete (entry);
				
				// to check for connection limit
				if (sock >= 0 && g_shpTmap->GetNumConnections () > g_maxCnct) {
#ifndef NDEBUG
					syslog (LOG_WARNING, "Number of connections have exceeded limit of: %d\n", g_maxCnct);
#endif
					close (sock);
					sock = -1;
				}
				
				// process new client socket
				if (sock >= 0) {
					pthread_t pthId;
					
#ifndef NDEBUG
					syslog (LOG_WARNING, "AcceptConnection has returned a socket value: %d\n", sock);
#endif
					
					ShpParams taskPars (new TASK_PARAMETERS (sock, authData));
					ShpTask netTask (new CNetworkTask (taskPars));
					if (0 != (ret = netTask->Start (&pthId, NULL))) {
#ifndef NDEBUG
						syslog (LOG_ERR, "Error of pthread_create: %s, code: %d; file: %s - line: %d\n",
								StrError (ret).c_str (), ret, __FILE__, __LINE__
						);
#endif
						return ret;
					}
					int tmp;
					//pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, &tmp);
					//pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &tmp);
					g_shpTmap->Insert (pthId, netTask);
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
					g_shpTmap->IterateEntries (
						[] (std::pair <const pthread_t, std::shared_ptr <CTask>> & entry)->void {pthread_kill (entry.first, SIGUSR2);}
					);
					g_shpTmap->Clear ();
					close (sockLstn);
					sleep (1);
					
					break;
				} else if (g_flgDat.CheckTerm ()) {
					g_flgDat.SetTerm (false);
#ifndef NDEBUG
					syslog (LOG_WARNING, "Normal termination of the server\n");
#endif
					pthread_kill (thrSigHnd, SIGTERM);
					g_shpTmap->IterateEntries (
						[] (std::pair <const pthread_t, std::shared_ptr <CTask>> & entry)->void {pthread_kill (entry.first, SIGUSR2);}
					);
					g_shpTmap->Clear ();
					close (sockLstn);
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
		syslog (LOG_ERR, "Has caught an exception at root function: %s\n", Exc.what ());
#endif
		return 101;
	}
	
	
	return 0;
}


void* CHandlerTask::WorkerFunction () {
	int ret, sigRet;
	sigset_t sigMsk;
	
	//
	// mask of signals adjusting
	//
	sigfillset (&sigMsk);
	if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
		ret = errno;
#ifndef NDEBUG
	syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d\n", StrError (ret).c_str (), ret);
#endif
		return NULL;
	}
	
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


int CNetworkTask::CommandsCheck (int sock, const std::string & cmd) {
	int ret;
	ssize_t retLen;
	const std::string & hloStr (getParams ().helloStr);

#ifndef NDEBUG
	syslog (LOG_NOTICE, "Has gotten a command: %s", cmd.c_str ());
#endif
	// have gotten empty command
	if (cmd.length () == 0) {
		DATA_HEADER datInf (
			hloStr.length (),
			DATA_HEADER::ServerAnswer | DATA_HEADER::ServerRequest,
			DATA_HEADER::Negative | DATA_HEADER::CanContinue,
			DATA_HEADER::NoStatus
		);
		if (-1 == (retLen = WriteToSock (sock, ('\n' + hloStr).c_str (), 0, datInf)) || -2 == retLen) {
			return -1;
		}
		return 1;
	}
	if (cmd == "exit") {
		std::string byeStr ("Bye bye\n");
		DATA_HEADER datInf (
			byeStr.length (),
			DATA_HEADER::ServerAnswer,
			DATA_HEADER::Positive,
			DATA_HEADER::InteractionFin
		);
		WriteToSock (sock, byeStr.c_str (), 0, datInf);
		return 2;
	}
	
	return 0;
}


int CNetworkTask::AdjustSignals () {
	sigset_t sigMsk;
	int ret;
	
	sigfillset (&sigMsk);
	sigdelset (&sigMsk, SIGUSR2);
	if (-1 == sigprocmask (SIG_SETMASK, &sigMsk, NULL)) {
		ret = errno;
#ifndef NDEBUG
	    syslog (LOG_ERR, "Error of sigprocmask: %s, code: %d, in file: %s, at string: %d\n", 
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	return 0;
}


int CNetworkTask::CheckAuthInfo (int sock) {
	ssize_t retLen;
	std::vector <char> datBuf (1024);
	int ret;
	
	
	//
	// login check
	//
	std::unordered_map <std::string, std::string>::const_iterator it;
	{
		DATA_HEADER hdrInf (
			strlen (g_helloStr1),
			DATA_HEADER::ServerRequest,
			DATA_HEADER::Positive,
			DATA_HEADER::NoStatus
		);
		
		if (0 >= WriteToSock (sock, g_helloStr1, 0, hdrInf)) return -1;
		if (0 > (retLen = ReadFromSock (sock, datBuf, 0, hdrInf))) return -1;
		if (0 != (ret = CheckFormatHeader (hdrInf))) {
#ifndef NDEBUG
			syslog (LOG_NOTICE, "CheckFormatHeader has returned: %d\n", ret);
#endif
			WriteToSock (sock, NULL, 0, hdrInf);
			return -2;
		}
		if (0 != (ret = CheckLogicallyHeader (hdrInf))) {
#ifndef NDEBUG
			syslog (LOG_NOTICE, "CheckLogicallyHeader has returned: %d\n", ret);
#endif
			WriteToSock (sock, NULL, 0, hdrInf);
			return -2;
		}
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten user name: %s\n", &datBuf[0]);
#endif	
		
		it = getParams ().authData.find (&datBuf[0]);
		if (it == getParams ().authData.end ()) {
			hdrInf.ZeroStruct ();
			hdrInf.u.dat.dataLen = 0;
			hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer;
			hdrInf.u.dat.retStatus = DATA_HEADER::Negative;
			hdrInf.u.dat.retExtraStatus = DATA_HEADER::BadName;
			WriteToSock (sock, NULL, 0, hdrInf);
			return -1;
		}
	}
	
	//
	// pass check
	//
	{
		DATA_HEADER hdrInf (
			strlen (g_helloStr2),
			DATA_HEADER::ServerRequest,
			DATA_HEADER::Positive,
			DATA_HEADER::NoStatus
		);
		
		if (0 >= WriteToSock (sock, g_helloStr2, 0, hdrInf)) return -1;
		if (0 >= (retLen = ReadFromSock (sock, datBuf, 0, hdrInf))) return -1;
		if (0 != (ret = CheckFormatHeader (hdrInf))) {
#ifndef NDEBUG
			syslog (LOG_NOTICE, "CheckFormatHeader has returned: %d\n", ret);
#endif
			WriteToSock (sock, NULL, 0, hdrInf);
			return -2;
		}
		if (0 != (ret = CheckLogicallyHeader (hdrInf))) {
#ifndef NDEBUG
			syslog (LOG_NOTICE, "CheckLogicallyHeader has returned: %d\n", ret);
#endif
			WriteToSock (sock, NULL, 0, hdrInf);
			return -2;
		}
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten password: %s\n", &datBuf[0]);
#endif	
		
		if (it->second != &datBuf[0]) {
			hdrInf.ZeroStruct ();
			hdrInf.u.dat.dataLen = 0;
			hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer;
			hdrInf.u.dat.retStatus = DATA_HEADER::Negative;
			hdrInf.u.dat.retExtraStatus = DATA_HEADER::BadPass;
			WriteToSock (sock, NULL, 0, hdrInf);
			return -1;
		}
		getParams ().helloStr = it->first + ": ";
	}
	
	
	return 0;
}


int CNetworkTask::ReadFromDescriptor (int rdFd, std::vector <char> & outStr, int waitMilSec) {
	ssize_t totLen = 0, curLen;
	int ret, flVal, timeWaitMilSec = 100;
	
	//
	// File object flags adjusting
	//
	if ((flVal = fcntl (rdFd, F_GETFL, 0)) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of fcntl (get file flags): %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	if (fcntl (rdFd, F_SETFL, flVal | O_NONBLOCK) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of fcntl (set file flags): %s, code: %d, at file: %s, at line: %d\n",
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
			syslog (LOG_ERR, "Error of read: %s, code: %d, at file: %s, at line: %d\n",
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


int CNetworkTask::SetIdsAsUser (const std::string & userName) {
	// getpwent, getpwnam
	struct passwd pwdDat, *resDat;
	std::vector <char> chArr (4096 + 1);
	int ret;
	
	getpwnam_r (userName.c_str (), &pwdDat, &chArr[0], chArr.size () - 1, &resDat);
	if (resDat == NULL) return 1;
	
	if (-1 == setresuid (pwdDat.pw_uid, pwdDat.pw_uid, pwdDat.pw_uid)) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of setresuid: %s, code: %d, at file: %s, at "
		"line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return 2;
	}
	
	return 0;
}


int CNetworkTask::Pipe (
	const std::vector <std::string> & cmdStr,
	std::vector <char> & outStr,
	int & retValue,
	const std::string & userName
)
{
	int ret, pid, pipeFd[2];
	struct sigaction sigAct;
	sigset_t sigMask;
	
	
	if (-1 == pipe (pipeFd)) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of pipe: %s, code: %d, at file: %s, at line: %d\n",
				StrError (ret).c_str (), ret, __FILE__, __LINE__
		);
#endif
		return -1;
	}
	
	if ((pid = fork ()) < 0) // error
	{
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of fork: %s, code: %d, at file: %s, at line: %d\n",
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
		if (0 > (ret = ReadFromDescriptor (pipeFd[0], outStr, g_totalWaitPid))) {
			if (!kill (pid, SIGKILL)) wait (NULL);
			close (pipeFd[0]);
			return -1;
		}
		close (pipeFd[0]);
		
		while (waitMilSec < g_totalWaitPid / 10)
		{
			if (-1 == (ret = waitpid (pid, &retStatus, WNOHANG))) {
#ifndef NDEBUG
				ret = errno;
				syslog (LOG_ERR, "Error of waitpid: %s, code: %d, at file: %s, at line: %d\n",
						StrError (ret).c_str (), ret, __FILE__, __LINE__
				);
#endif
				if (!kill (pid, SIGKILL)) wait (NULL);
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
		syslog (LOG_ERR, "Time at waitpid have elapsed, file: %s, line: %d\n", __FILE__, __LINE__);
#endif
		if (!kill (pid, SIGKILL)) wait (NULL);
		return -1;
	} else // (pid == 0) // child
	{
		close (pipeFd[0]);
		sigemptyset (&sigMask);
		
		if (sigprocmask (SIG_SETMASK, &sigMask, NULL) == -1) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of sigprocmask at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		// stdin
		close (0);
		if (-1 == dup2 (pipeFd[1], 1) || -1 == dup2 (pipeFd[1], 2)) { // stdout and stderr
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of dup2 at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		// To check for execution by of name other user
		if (userName != "") {
			if (SetIdsAsUser (userName)) return -1;
		}
		
		char *parArr [g_maxParLen + 1];	
		for (int i = 0; i < cmdStr.size (); ++i) parArr [i] = const_cast <char*> (cmdStr[i].c_str ());
		parArr[cmdStr.size ()] = NULL;
		if (-1 == execvp (cmdStr [0].c_str (), parArr)) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of execlp at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
	}
	
	
	return -2;
}


int CNetworkTask::ParseParameters (
	const std::vector <char> & chBuf,
	std::vector <std::string> & parStrs,
	bool asUser, // if as user, so last substring is a username
	std::string & userName
)
{
	std::string tmpStr (&chBuf[0]);
	std::string::size_type pos1 = 0, pos2;
	int cnt = 0;
	
	
	while ((pos1 = tmpStr.find (' ', pos1)) != std::string::npos) {
		while (tmpStr[pos1] == ' ') ++pos1;
		++cnt;
	}
	if (cnt > g_maxParLen) return -1;
	
	if (asUser) {
		if (std::string::npos == (pos1 = tmpStr.find_last_of (' '))) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Can't find username at parameters, file: %s, line: %d\n", __FILE__, __LINE__);
#endif
		} else {
			while (tmpStr[pos1] == ' ') ++pos1;
			parStrs.push_back (tmpStr.substr (pos1, tmpStr.length () - pos1));
		}
	} else userName = "";
	
	pos1 = 0;
	do {
		while (tmpStr[pos1] == ' ' && tmpStr[pos1] != '"' && tmpStr[pos1] != '\'') ++pos1;
		if (tmpStr[pos1] == '"')
		{
			if ((pos2 = tmpStr.find ('"', pos1 + 1)) == std::string::npos) pos2 = tmpStr.size ();
			else pos2++;
		} 
		else if (tmpStr[pos1] == '\'')
		{
			if ((pos2 = tmpStr.find ('\'', pos1 + 1)) == std::string::npos) pos2 = tmpStr.size ();
			else pos2++;
		}
		else
		{
			// if it's the last iteration we don't decrement pos2
			if ((pos2 = tmpStr.find (' ', pos1)) == std::string::npos) pos2 = tmpStr.size ();
		}
		parStrs.push_back (tmpStr.substr (pos1, pos2 - pos1));
		
#ifndef NDEBUG
		syslog (LOG_ERR, "One substring: %s\n", tmpStr.substr (pos1, pos2 - pos1).c_str ());
#endif
		
		pos1 = pos2;
	} while ((pos1 = tmpStr.find (' ', pos1)) != std::string::npos);
	
	
	return 0;
}


void* CNetworkTask::WorkerFunction () {
	int ret, retStatus;
	ssize_t retLen, curLen;
	int sock = getParams ().sock;
	std::string hloStr;
	const int currentSize = 4096;
	std::vector <char> chBuf (currentSize);
	
	
	assert (sock != 0);
	
	//
	// mask of signals adjusting
	//
	if (-1 == AdjustSignals ()) {
		close (sock);
		return NULL;
	}
	
	//
	// To check auth information
	//
	if (0 > CheckAuthInfo (sock)) {
		close (sock);
		return NULL;
	}
	hloStr = getParams ().helloStr;
	
	assert (hloStr.length () != 0);
	
	//
	// To send hello str and enter at main loop
	//
	DATA_HEADER hdrInf (
		hloStr.length (),
		DATA_HEADER::ServerRequest,
		DATA_HEADER::Positive,
		DATA_HEADER::NoStatus
	);
	if (-1 == (retLen = WriteToSock (sock, hloStr.c_str (), 0, hdrInf)) || -2 == retLen) {
		close (sock);
		return NULL;
	}
	
	try {
		while (true) {
			hdrInf.ZeroStruct ();
			std::vector <std::string> parStrs;
			std::string strErr ("While executing a command have happened an error, command: ");
			std::string userName ("");
			
			if (0 > (retLen = ReadFromSock (sock, chBuf, 0, hdrInf))) {
				close (sock);
				return NULL;
			}
			if (0 != (ret = CheckFormatHeader (hdrInf))) {
#ifndef NDEBUG
				syslog (LOG_NOTICE, "CheckFormatHeader has returned: %d\n", ret);
#endif
				WriteToSock (sock, NULL, 0, hdrInf);
				close (sock);
				return NULL;
			}
			if (0 != (ret = CheckLogicallyHeader (hdrInf))) {
#ifndef NDEBUG
				syslog (LOG_NOTICE, "CheckLogicallyHeader has returned: %d\n", ret);
#endif
				WriteToSock (sock, NULL, 0, hdrInf);
				close (sock);
				return NULL;
			}
			chBuf[retLen] = '\0';
			
			//
			// check command
			//
			if ((ret = CommandsCheck (sock, &chBuf[0])) == -1) {
				close (sock);
				return NULL;
			} else if (ret == 1) {
				continue;
			} else if (ret == 2) {
				close (sock);
				return NULL;
			}
			
			strErr += &chBuf[0];
			if (-1 == ParseParameters (
				chBuf,
				parStrs,
				hdrInf.u.dat.retExtraStatus == DATA_HEADER::RunAsUser,
				userName)
			) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Too many parameters have gottent at request\n");
#endif
				hdrInf.u.dat.dataLen = 0;
				hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer;
				hdrInf.u.dat.retStatus = DATA_HEADER::Negative;
				hdrInf.u.dat.retExtraStatus = DATA_HEADER::NoStatus;
				WriteToSock (sock, NULL, 0, hdrInf);
				close (sock);
				return NULL;
			}
			
			if ((ret = Pipe (parStrs, chBuf, retStatus, userName)) == -1) {
				strcpy (&chBuf[0], "Internal server error\n");
				hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer | DATA_HEADER::ServerRequest;
				hdrInf.u.dat.retStatus = DATA_HEADER::Negative | DATA_HEADER::CanContinue;
				hdrInf.u.dat.retExtraStatus = DATA_HEADER::InternalServerError;
			} else if (ret == -2) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Pipe function has returned -2, file: %s, line: %d\n", __FILE__, __LINE__);
#endif
				assert (1 != 1);
			} else {
				hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer | DATA_HEADER::ServerRequest;
				hdrInf.u.dat.retStatus = DATA_HEADER::Positive;
				hdrInf.u.dat.retExtraStatus = DATA_HEADER::NoStatus;
			}
			
			std::string tmpStr (&chBuf[0]);
			if (tmpStr [tmpStr.size () - 1] != '\n') tmpStr += '\n';
			tmpStr += hloStr;
			hdrInf.u.dat.dataLen = tmpStr.size ();
			
			if ((0 > WriteToSock (sock, tmpStr.c_str (), 0, hdrInf))) {
				close (sock);
				return NULL;
			}
			chBuf.resize (currentSize);
		}
	} catch (std::exception & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Have caught an exception: %s\n", Exc.what ());
#endif
		DATA_HEADER hdrInf (
			0,
			DATA_HEADER::ServerAnswer,
			DATA_HEADER::Negative,
			DATA_HEADER::InternalServerError
		);
		WriteToSock (sock, NULL, 0, hdrInf);
		close (sock);
		return NULL;
	}
	
	
	return NULL;
}


int CheckFormatHeader (DATA_HEADER & hdrInf) {
	int lenDat, cmdType, retStatus, retExtraStatus;
	
	lenDat = hdrInf.u.dat.dataLen;
	cmdType = hdrInf.u.dat.cmdType;
	retStatus = hdrInf.u.dat.retStatus;
	retExtraStatus = hdrInf.u.dat.retExtraStatus;
	
	// format checks
	if (cmdType < DATA_HEADER::ExecuteCommand ||
		cmdType > DATA_HEADER::CmdsSUMMARY) return 2;
	if (retStatus < DATA_HEADER::Positive ||
		retStatus > DATA_HEADER::StatusesSUMMARY) return 3;
	if (retExtraStatus < DATA_HEADER::NoStatus ||
		retExtraStatus > DATA_HEADER::RunAsUser) return 4;
	
	return 0;
}


int CheckLogicallyHeader (DATA_HEADER & hdrInf) {
	int lenDat, cmdType, retStatus, retExtraStatus;
	
	lenDat = hdrInf.u.dat.dataLen;
	cmdType = hdrInf.u.dat.cmdType;
	retStatus = hdrInf.u.dat.retStatus;
	retExtraStatus = hdrInf.u.dat.retExtraStatus;
	
	// length check - this check need move to logically checks
	if (lenDat > DATA_HEADER::MaxDataLen || lenDat < 0) return 1;
	
	// other checks of the fields
	
	return 0;
}










