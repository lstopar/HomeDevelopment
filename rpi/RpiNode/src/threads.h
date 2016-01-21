/*
 * threads.h
 *
 *  Created on: Jan 21, 2016
 *      Author: lstopar
 */

#ifndef SRC_THREADS_H_
#define SRC_THREADS_H_

#include "base.h"
#include <pthread.h>


/**
 * Critical section - Allows only 1 thread to enter, but the same thread can enter multiple times
 */
class TCriticalSection {
protected:
	//CRITICAL_SECTION Cs;
	pthread_mutex_t Cs;
	pthread_mutexattr_t CsAttr;

public:
	TCriticalSection();
	~TCriticalSection();

	void Enter();
	bool TryEnter();
	void Leave();
};

////////////////////////////////////////////
// Lock
//   Wrapper around critical section, which automatically enters
//   on construct, and leaves on scope unwinding (destruct)
class TLock {
	friend class TCondVarLock;
private:
	TCriticalSection& CriticalSection;
public:
	TLock(TCriticalSection& _CriticalSection):
		CriticalSection(_CriticalSection) { CriticalSection.Enter(); }
	~TLock() { CriticalSection.Leave(); }
};


#endif /* SRC_THREADS_H_ */
