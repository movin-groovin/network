
#ifndef NETWORK_SERVER_OWN
#define NETWORK_SERVER_OWN

#include "../mon_daemon/daemonize.hpp"

#include <memory>
#include <unordered_map>
#include <exception>
#include <vector>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>


//
// Function prototypes
//
std::string StrError (int errCode);
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
	pthread_mutex_t m_syncQueue;
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
};


typedef struct _TASK_PARAMETERS {
public:
	_TASK_PARAMETERS (int sck = 0, const std::string & str): sock (sck), helloStr (str) {}

public:
	std::string helloStr;
	int sock;
} TASK_PARAMETERS, *PTASK_PARAMETERS;


class CTask {
public:
	typedef std::shared_ptr <TASK_PARAMETERS> ParamPars;

private:
	std::shared_ptr <TASK_PARAMETERS> m_tskParams;

protected:
	const std::shared_ptr <TASK_PARAMETERS> & getParams () const {
		return m_tskParams;
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
	
};

/*
typedef struct _COND_DATA {
	pthread_mutex_t syncMut;
	pthread_cond_t syncCond;
	
	_COND_DATA () {
		int ret;
		std::istringstream issCnv;
		
		if ((ret = pthread_mutex_init (&syncMut, NULL)) != 0) {
			issCnv << ret;
			throw CException ("Error of pthread_mutex_init, description: " + 
							  StrError (ret) + "; code: " + issCnv.str () + "\n"
			);
		}
		if ((ret = pthread_cond_init (&syncCond, NULL)) != 0) {
			pthread_mutex_destroy (&syncMut);
			issCnv << ret;
			throw CException ("Error of pthread_cond_init, description: " + 
							  StrError (ret) + "; code: " + issCnv.str () + "\n"
			);
		}
		
		return;
	}
	~ _COND_DATA () {
		pthread_mutex_destroy (&syncMut);
		pthread_cond_destroy (&syncCond);
	}
	
} COND_DATA, *PCOND_DATA;
*/

typedef class _FLAGS_DATA {
	pthread_spinlock_t m_guard;
	bool m_term, m_reread;
	
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
	union {
		char arr [256];
		struct {
			int dataLen; // the lenght of useful data, behind the header
			// other info
		} dat;
	} u;
} DATA_HEADER, *PDATA_HEADER;


//
// Typedef declaration
//
typedef typename CTask::ParamPars ShpParams;
//typedef std::shred_ptr <COND_DATA> ShpCondWait;
typedef std::shared_ptr <CTask> ShpTask;
typedef std::shared_ptr <CTaskMap> ShpTskMap;



#endif // NETWORK_SERVER_OWN
