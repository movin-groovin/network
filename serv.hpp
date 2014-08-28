
#ifndef NETWORK_SERVER_OWN
#define NETWORK_SERVER_OWN

#include "../mon_daemon/daemonize.hpp"

#include <memory>
#include <unordered_map>
#include <exception>
#include <vector>

#include <cassert>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <sys/wait.h>



//
// Function prototypes
//
std::string StrError (int errCode);
//
// CallFunc is a friend function of class CTask and defined there
//
void* CallFunc (void * pvPtr);


//
// Class  declarations
//

class CException {
private:
	std::string m_msgStr;
	
public:
	CException (const std::string & msgStr): m_msgStr (msgStr) {}
	const char* what () const noexcept {
		return "CException class: ";
	}
	std::string MsgError () const {
		return what () + m_msgStr;
	}
};


class CTask;

class CTaskMap {
private:
	mutable pthread_mutex_t m_syncQueue;
	std::unordered_map <pthread_t, std::shared_ptr <CTask> > m_tskQueue;
	
public:
	CTaskMap () {
		int ret;
		
		if ((ret = pthread_mutex_init (&m_syncQueue, NULL)) != 0) {
			throw CException (StrError (ret));
		}
		
		return;
	}
	
	~CTaskMap () {
		pthread_mutex_destroy (&m_syncQueue);
	}
	
	template <typename Func>
	void IterateEntries (Func f) {
		pthread_mutex_lock (&m_syncQueue);
		for (auto & entry : m_tskQueue) f (entry);
		pthread_mutex_unlock (&m_syncQueue);
		return;
	}
	
	void Insert (pthread_t pthId, std::shared_ptr <CTask> par) {
		pthread_mutex_lock (&m_syncQueue);
		m_tskQueue.insert (std::make_pair (pthId, par));
		pthread_mutex_unlock (&m_syncQueue);
	}
	void Delete (pthread_t pthId) {
		pthread_mutex_lock (&m_syncQueue);
		m_tskQueue.erase (pthId);
		pthread_mutex_unlock (&m_syncQueue);
	}
	void Clear () {
		pthread_mutex_lock (&m_syncQueue);
		m_tskQueue.clear ();
		pthread_mutex_unlock (&m_syncQueue);
	}
	size_t GetNumConnections () const {
		pthread_mutex_lock (&m_syncQueue);
		size_t ret = m_tskQueue.size ();
		pthread_mutex_unlock (&m_syncQueue);
		return ret;
	}
};


typedef struct _TASK_PARAMETERS {
public:
	_TASK_PARAMETERS (
		int sck = 0,
		const std::unordered_map <std::string, std::string> & authDat = 
			std::unordered_map <std::string, std::string> (),
		const std::string & str = ""
	): sock (sck), helloStr (str), authData (authDat) {}

public:
	std::string helloStr;
	int sock;
	std::unordered_map <std::string, std::string> authData;
} TASK_PARAMETERS, *PTASK_PARAMETERS;


class CTask {
	friend void* CallFunc (void * pvPtr) {
		return static_cast <CTask*> (pvPtr)->WorkerFunction ();
	}
	
public:
	typedef std::shared_ptr <TASK_PARAMETERS> ParamPars;

private:
	std::shared_ptr <TASK_PARAMETERS> m_tskParams;

protected:
	TASK_PARAMETERS & getParams () const {
		return *m_tskParams;
	}
	CTask (ParamPars pars): m_tskParams (pars) {}
	
public:
	virtual ~CTask () {}
	int Start (pthread_t *thread, const pthread_attr_t *attr) {
		int ret;
		
		// here in WorkerFunction *p and *arg are equal
		ret = pthread_create (thread,
							  attr,
							  &CallFunc,
							  this
		);
		
		return ret;
	}
	virtual void* WorkerFunction () = 0;
};


class CHandlerTask: public CTask {
private:
	
	
public:
	CHandlerTask (ParamPars pars): CTask (pars) {}
	void* WorkerFunction ();
	
};


class CNetworkTask: public CTask {
private:
	
	
public:
	CNetworkTask (ParamPars pars): CTask (pars) {}
	void* WorkerFunction ();
	
	int CommandsCheck (int sock, const std::string & cmd);
	int AdjustSignals ();
	int CheckAuthInfo (int sock);
	int Pipe (const std::vector <std::string> & cmdStr, std::vector <char> & outStr, int & retValue);
	int ReadFromDescriptor (int rdFd, std::vector <char> & outStr, int waitMilSec);
	int ParseParameters (const std::vector <char> & chBuf, std::vector <std::string> & parStrs);
	int SetIdsAsUser (const std::string & userName);
};


typedef class _FLAGS_DATA {
private:
	mutable pthread_spinlock_t m_guard;
	bool m_term, m_reread;

public:
	_FLAGS_DATA () {
		pthread_spin_init (&m_guard, 0);
	}
	~ _FLAGS_DATA () {
		pthread_spin_destroy (&m_guard);
	}

	bool CheckReread () const {
		bool tmp;
		pthread_spin_lock (&m_guard);
		tmp = m_reread;
		pthread_spin_unlock (&m_guard);
		return tmp;
	}
	void SetReread (bool val) {
		pthread_spin_lock (&m_guard);
		m_reread = val;
		pthread_spin_unlock (&m_guard);
	}
	
	bool CheckTerm () const {
		bool tmp;
		pthread_spin_lock (&m_guard);
		tmp = m_term;
		pthread_spin_unlock (&m_guard);
		return tmp;
	}
	void SetTerm (bool val) {
		pthread_spin_lock (&m_guard);
		m_term = val;
		pthread_spin_unlock (&m_guard);
	}
} FLAGS_DATA, *PFLAGS_DATA;


typedef struct _DATA_HEADER {
	//
	// Information interchange between client and server exist of 256
	// bytes header and useful data straight after the header
	//
	static const int MaxDataLen = 128 * 1024 * 1024;
	enum Cmds {ExecuteCommand = 0x0, ServerAnswer = 0x1,
			   ServerRequest = 0x2, ClientAnswer = 0x4,
			   ClientRequest = 0x8, CmdsSUMMARY = 0x0+0x1+0x2+0x4+0x8
	};
	enum Statuses {Positive = 0x0, Negative = 0x1,
				   CanContinue = 0x2, StatusesSUMMARY = 0x0+0x1+0x2
	};
	enum ExtraStatuses {NoStatus = 0, BadName = 1,
						BadPass = 2, InternalServerError = 3,
						TooLong = 4, InteractionFin = 5
	};
	
	union {
		char arr [256];
		struct {
			int dataLen; // the lenght of useful data, behind the header
			// command type
			// 1 - user request to execute by child process
			// 2 - user request to execute by network server
			// 3 - server answer
			// 4 - server request of client
			int cmdType;
			int retStatus; // 1 - positive, 2 - negative
			int retExtraStatus; // extra information about request or returned reply
			// other info
		} dat;
	} u;
	
	_DATA_HEADER (
		int lenDat,
		int cmdInf = 0,
		int retInf = 0,
		int statInf = 0
	)
	{
		ZeroStruct ();
		u.dat.dataLen = lenDat;
		u.dat.cmdType = cmdInf;
		u.dat.retStatus = retInf;
		u.dat.retExtraStatus = statInf;
	}
	
	void ZeroStruct () {memset (&u, 0, sizeof (_DATA_HEADER));}
} DATA_HEADER, *PDATA_HEADER;

/*
template <typename T, typename RelFunc>
class CResourceReleaser {
public:
	void operation () (T *ptr) {
		if (*ptr >= 0) RelFunc (*ptr);
		delete ptr;
	}
};
*/

//
// Typedef declaration
//
typedef typename CTask::ParamPars ShpParams;
//typedef std::shred_ptr <COND_DATA> ShpCondWait;
typedef std::shared_ptr <CTask> ShpTask;
typedef std::shared_ptr <CTaskMap> ShpTskMap;


//
// Function prototypes
//
int CheckFormatHeader (DATA_HEADER & hdrInf);
int CheckLogicallyHeader (DATA_HEADER & hdrInf);




#endif // NETWORK_SERVER_OWN
