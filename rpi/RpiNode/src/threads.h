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

class TBlocker {
protected:
	pthread_cond_t Event;
	pthread_mutex_t Mutex;

	//HANDLE Event;

public:
	TBlocker();
	~TBlocker();

	void Block(int Msecs = TInt::Mx);
	void Release();
};

////////////////////////////////////////////
// Thread
ClassTP(TThread, PThread)// {
private:
	const static int STATUS_CREATED;
	const static int STATUS_STARTED;
	const static int STATUS_CANCELLED;
	const static int STATUS_FINISHED;

	pthread_t ThreadHandle;

	// Use for interrupting and waiting
	TBlocker* SleeperBlocker;
	TCriticalSection CriticalSection;

	volatile sig_atomic_t Status;

private:
    static void * EntryPoint(void * pArg);
    static void SetFinished(void *pArg);
public:
    TThread();

	TThread(const TThread& Other) {
		operator=(Other);
	}
	TThread& operator =(const TThread& Other);

    virtual ~TThread();

    // starts the thread
    void Start();
    // when started the thread calls this function
    virtual void Run() { printf("empty run\n"); };

    // terminates the thread
    void Cancel();

    // windows thread id
    uint64 GetThreadId() const { return (uint64)GetThreadHandle(); }
    // windows thread handle
    pthread_t GetThreadHandle() const { return ThreadHandle; }

	void Interrupt();
	void WaitForInterrupt(const int Msecs = INFINITE);

	int Join();

	bool IsAlive();
	bool IsCancelled();
	bool IsFinished();

	static int GetCoreCount();

};


#endif /* SRC_THREADS_H_ */
