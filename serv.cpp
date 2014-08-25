
// gcc -lstdc++ -std=c++11 -lpthread serv.cpp ../mon_daemon/daemonize.cpp -o network_server

#include "serv.hpp"



const int maxCnct = 10, g_sleepAcceptTimeout = 10, g_sleepWorkerThread = 1; // in seconds
const char *masterPassw = "c54ccf798";
const char *masterName = "master";
const char *confPath = "/home/yarr/src/srv/network/network_server.conf";
const char *tcp4Data = "/proc/net/tcp";
const char *helloStr1 = "Enter a name: ";
const char *helloStr2 = "Enter a password: ";
const char *shadowConf = "/etc/1234DEADBEAF4321/tcp4.txt";
const int maxParLen = 15;
const int totalWaitPid = 128 * 100; // 12.8 sec > 10 sec, values at milliseconds

ShpTskMap g_shpTmap (new CTaskMap);
FLAGS_DATA g_flgDat;



std::string StrError (int errCode) {
	std::vector <char> tmp (1024 + 1);
	
	return std::string (strerror_r (errCode, &tmp[0], 1024));
}


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
	authData.insert (std::make_pair (masterName, masterPassw));
	
	
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
#ifndef NDEBUG
	assert (0 != inet_aton ("127.0.0.1", &sockAddr.sin_addr));
#else
	sockAddr.sin_addr.s_addr = htonl (INADDR_ANY);
#endif
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
	
	if (-1 == listen (sock, maxCnct / 2)) {
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


int main (int argc, char *argv []) {
	try {
		int ret, sockLstn;
		const char *chName = "network_server";
		ShpTask hndTask (new CHandlerTask (ShpParams (new TASK_PARAMETERS)));
		pthread_t thrSigHnd;
		
		//
		// daemonize and logging
		//
		if ((ret = daemonize (chName)) != 0) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of daemonize, ret code: %d\n", ret);
#endif
			return ret;
		}
		
		if (argc < 3) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Need the config path and special string\n");
#endif
			return 0;
		}
		confPath = argv[1];

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
			std::ifstream iFs (confPath);
			if (!iFs) {
				ret = errno;
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of file opening: %s, code: %d, file: %s\n",
						StrError (ret).c_str (), ret, confPath
				);
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
				//
				// To accept connection and add to shadow config
				//
				int sock = AcceptConnection (sockLstn, shadowConf);
				// -1 - error, -2 - have gotten SIGUSR2 or have elapsed timeout, 0 or more - successful result
				if (sock >= 0) {
					pthread_t pthId;
					
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
		DATA_HEADER datInf (hloStr.length (), DATA_HEADER::ServerRequest, 0, DATA_HEADER::WaitCommand);
		if (-1 == (retLen = WriteToSock (sock, hloStr.c_str (), 0, datInf)) || -2 == retLen)
		{
			close (sock);
			return -1;
		}
		return 1;
	}
	if (cmd == "exit") {
		std::string byeStr ("Bye bye\n");
		DATA_HEADER datInf (byeStr.length (), DATA_HEADER::ServerAnswer, DATA_HEADER::Positive, 0);
		WriteToSock (sock, byeStr.c_str (), 0, datInf);
		close (sock);
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
	
	
	//
	// login check
	//
	std::unordered_map <std::string, std::string>::const_iterator it;
	{
		DATA_HEADER hdrInf (strlen (helloStr1));
		hdrInf.u.dat.cmdType = DATA_HEADER::ServerRequest;
		hdrInf.u.dat.statusInfo = DATA_HEADER::GetName;
		
		if (0 >= WriteToSock (sock, helloStr1, 0, hdrInf)) return -1;
		if (0 > (retLen = ReadFromSock (sock, datBuf, 0, hdrInf))) return -1;
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten user name: %s\n", &datBuf[0]);
#endif	
		
		it = getParams ().authData.find (&datBuf[0]);
		if (it == getParams ().authData.end ()) {
			hdrInf.ZeroStruct ();
#ifndef NDEBUG
			std::string errStr ("Incorrect user name\n");
#else
			std::string errStr ("");
#endif
			hdrInf.u.dat.dataLen = errStr.size ();
			hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer;
			hdrInf.u.dat.retStatus = DATA_HEADER::Negative;
			hdrInf.u.dat.statusInfo = DATA_HEADER::BadName;
			WriteToSock (sock, errStr.c_str (), 0, hdrInf);
			return -1;
		}
	}
	
	//
	// pass check
	//
	{
		DATA_HEADER hdrInf (strlen (helloStr2));
		hdrInf.u.dat.cmdType = DATA_HEADER::ServerRequest;
		hdrInf.u.dat.statusInfo = DATA_HEADER::GetPass;
		
		if (0 >= WriteToSock (sock, helloStr2, 0, hdrInf)) return -1;
		if (0 >= (retLen = ReadFromSock (sock, datBuf, 0, hdrInf))) return -1;
		datBuf[retLen] = '\0';
#ifndef NDEBUG
		syslog (LOG_WARNING, "Have gotten password: %s\n", &datBuf[0]);
#endif	
		
		if (it->second != &datBuf[0]) {
			hdrInf.ZeroStruct ();
#ifndef NDEBUG
			std::string errStr ("Incorrect password\n");
#else
			std::string errStr ("");
#endif
			hdrInf.u.dat.dataLen = errStr.size ();
			hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer;
			hdrInf.u.dat.retStatus = DATA_HEADER::Negative;
			hdrInf.u.dat.statusInfo = DATA_HEADER::BadPass;
			WriteToSock (sock, errStr.c_str (), 0, hdrInf);
			return -1;
		}
		getParams ().helloStr = it->first + ": ";
	}
	
	
	return 0;
}


int CNetworkTask::ReadFromDescriptor (int rdFd, std::vector <char> & outStr, int waitMilSec) {
	ssize_t totLen = 0, curLen;
	int ret;
	
	while ((curLen = read (rdFd, &outStr[0] + totLen, outStr.size () - 1 - totLen)) != 0) {
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
	}
	outStr[totLen] = '\0';
	
	return totLen;
}


int CNetworkTask::Pipe (const std::vector <std::string> & cmdStr, std::vector <char> & outStr, int & retValue) {
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
		if (0 > ReadFromDescriptor (pipeFd[0], outStr, totalWaitPid)) {
			kill (pid, SIGKILL);
			close (pipeFd[0]);
			return -1;
		}
		close (pipeFd[0]);
		
		while (waitMilSec < totalWaitPid / 10)
		{
			if (-1 == (ret = waitpid (pid, &retStatus, WNOHANG))) {
#ifndef NDEBUG
				ret = errno;
				syslog (LOG_ERR, "Error of waitpid: %s, code: %d, at file: %s, at line: %d\n",
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
		syslog (LOG_ERR, "Time at waitpid have elapsed, file: %s, line: %d\n", __FILE__, __LINE__);
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
			syslog (LOG_ERR, "Error of sigprocmask at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		close (0); // stdin
		if (-1 == dup2 (pipeFd[1], 1) || -1 == dup2 (pipeFd[1], 2)) { // stdout and stderr
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of dup2 at child process: %s, code: %d, at file:"
					" %s, at line: %d\n", StrError (ret).c_str (), ret, __FILE__, __LINE__
			);
#endif
			return 1;
		}
		
		char *parArr [maxParLen + 1];
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
	std::vector <std::string> & parStrs
)
{
	std::string tmpStr (&chBuf[0]);
	std::string::size_type pos1 = 0, pos2;
	int cnt = 0;
	
	
	while ((pos1 = tmpStr.find (' ', pos1)) != std::string::npos) {
		int locInd = 0;
		while (tmpStr[locInd] == ' ') ++pos1;
		++cnt;
	}
	if (cnt > maxParLen) return -1;
	
	pos1 = 0;
	while ((pos1 = tmpStr.find (' ', pos1)) != std::string::npos) {
		int locInd = 0;
		while (tmpStr[locInd] == ' ') ++pos1;
		// if it's the last iteration we don't decrement pos2
		if ((pos2 = tmpStr.find (' ', pos1)) == std::string::npos) pos2 = tmpStr.size ();
		parStrs.push_back (tmpStr.substr (pos1, pos2 - pos1));
#ifndef NDEBUG
		syslog (LOG_ERR, "One substring: %s\n", tmpStr.substr (pos1, pos2 - pos1).c_str ());
#endif
	}
	
	
	return 0;
}


void* CNetworkTask::WorkerFunction () {
	int ret;
	const int currentSize = 4096;
	ssize_t retLen, curLen;
	int sock = getParams ().sock;
	std::string hloStr = getParams ().helloStr;
	std::vector <char> chBuf (currentSize);
	FILE *pFl;
	
	
	assert (sock != 0);
	assert (hloStr.length () != 0);
	
	//
	// mask of signals adjusting
	//
	if (-1 == AdjustSignals ()) return NULL;
	
	//
	// To check auth information
	//
	if (-1 == CheckAuthInfo (sock)) {
		close (sock);
		return NULL;
	}
	
	//
	// To send hello str and enter at main loop
	//
	DATA_HEADER datInf (hloStr.length (), DATA_HEADER::ServerRequest, 0, DATA_HEADER::WaitCommand);
	if (-1 == (retLen = WriteToSock (sock, hloStr.c_str (), 0, datInf)) || -2 == retLen) {
		close (sock);
		return NULL;
	}
	
	try {
		while (true) {
			DATA_HEADER hdrInf (0);
			std::vector <std::string> parStrs;
			std::string strErr ("While executing a command have happened an error, command: ");
			
			if (0 > (retLen = ReadFromSock (sock, chBuf, 0, hdrInf))) {
				close (sock);
				return NULL;
			}
			chBuf[retLen] = '\0';
			
			//
			// check command
			//
			if ((ret = CommandsCheck (sock, &chBuf[0])) == -1) {
				return NULL;
			} else if (ret == 1) {
				continue;
			} else if (ret == 2) {
				return NULL;
			}
			
			strErr += &chBuf[0];
			if (-1 == ParseParameters (chBuf, parStrs)) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Too many parameters have gottent at request\n");
#endif
				close (sock);
				return NULL;
			}
/////////////////////////////////////////////////////////////
			pFl = popen (&chBuf[0], "r");
			if (pFl == NULL) {
				ret = errno;
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of popen: %s, code: %d, in file: %s, at string: %d, command: %s\n", 
						StrError (ret).c_str (), ret, __FILE__, __LINE__, &chBuf[0]
				);
#endif
				close (sock);
				return NULL;
			}
			
			int fd = fileno (pFl);
			if (fd == -1) {
				ret = errno;
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of fileno: %s, code: %d, in file: %s, at string: %d\n", 
						StrError (ret).c_str (), ret, __FILE__, __LINE__
				);
#endif
				close (sock);
				pclose (pFl);
				return NULL;
			}
			retLen = 0;
			while (true) {
				curLen = read (fd, &chBuf[0] + retLen, chBuf.size () - 1 - retLen);
				if (curLen == -1 && errno == EINTR) {
					std::string strInfo ("Connection was interrupted by server\n");
					//WriteToSock (sock, strInfo.c_str (), strInfo.size (), 0);
					close (sock);
					pclose (pFl);
					return NULL;
				}
				if (curLen == -1 && errno != EINTR) {
					//WriteToSock (sock, strError.c_str (), strError.size (), 0);
					close (sock);
					pclose (pFl);
					return NULL;
				}
				if (curLen == 0) break;
				
				retLen += curLen;
				if (retLen == chBuf.size () - 1) chBuf.resize (2 * chBuf.size ());
			}
			chBuf[retLen] = '\0';
/////////////////////////////////////////////////////////////
			
			std::string tmpStr (&chBuf[0]);
			if (tmpStr [tmpStr.size () - 1] != '\n') tmpStr += '\n';
			tmpStr += hloStr;
			
			hdrInf.u.dat.dataLen = tmpStr.size ();
			hdrInf.u.dat.cmdType = DATA_HEADER::ServerAnswer | DATA_HEADER::ServerRequest;
			hdrInf.u.dat.retStatus = DATA_HEADER::Positive;
			hdrInf.u.dat.statusInfo = DATA_HEADER::WaitCommand;
			
			if ((0 > WriteToSock (sock, tmpStr.c_str (), 0, hdrInf))) {
				close (sock);
				pclose (pFl);
				return NULL;
			}
			pclose (pFl);
			chBuf.resize (currentSize);
		}
	} catch (std::exception & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Have caught an exception: %s\n", Exc.what ());
#endif
		std::string errStr ("Internal server error");
		DATA_HEADER hdrInf (
			errStr.size (),
			DATA_HEADER::ServerAnswer,
			DATA_HEADER::Negative,
			DATA_HEADER::InternalServerError
		);
		WriteToSock (sock, errStr.c_str (), 0, hdrInf);
		close (sock);
		pclose (pFl);
		return NULL;
	}
	
	
	return NULL;
}










