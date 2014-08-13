
#ifndef NETWORK_SERVER_OWN
#define NETWORK_SERVER_OWN

#include "../mon_daemon/daemonize.hpp"

#include <memory>
#include <unordered_map>
#include <exception>
#include <vector>

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

class CTaskList {
private:
	pthread_mutex_t m_syncQueue;
	std::unordered_map <pthread_t, std::shared_ptr <CTask> > m_tskQueue;
	
public:
	CTaskList () {
		int ret;
		
		if ((ret = pthread_mutex_init (&m_syncQueue, NULL)) != 0) {
			throw CException (StrError (ret));
		}
		
		return;
	}
	
	~CTaskList () {
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
	_TASK_PARAMETERS (): sock (0) {}

public:
	int sock;
} TASK_PARAMETERS, *PTASK_PARAMETERS;


class CTask {
public:
	typedef std::shared_ptr <TASK_PARAMETERS> ParamPars;
	
private:
	std::shared_ptr <TASK_PARAMETERS> m_tskParams;
	void *argPtr;

protected:
	CTask (ParamPars pars): m_tskParams (pars), argPtr (NULL) {}
	
public:
	virtual ~CTask () {}
	int Start (pthread_t *thread, const pthread_attr_t *attr, void *arg) {
		int ret;
		
		// here in WorkerFunction *p and *arg are equal
		argPtr = arg;
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



//
// Typedef declaration
//
typedef typename CTask::ParamPars ShpParams;


#endif // NETWORK_SERVER_OWN
