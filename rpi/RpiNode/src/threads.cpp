#include "threads.h"

TCriticalSection::TCriticalSection() {
	pthread_mutexattr_init(&CsAttr);

	// allow the thread to enter thecritical section multiple times
	pthread_mutexattr_settype(&CsAttr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&Cs, &CsAttr);
}
TCriticalSection::~TCriticalSection() {
	pthread_mutex_destroy(&Cs);
	pthread_mutexattr_destroy(&CsAttr);
}
void TCriticalSection::Enter() {
	pthread_mutex_lock(&Cs);
}
bool TCriticalSection::TryEnter() {
	return pthread_mutex_trylock(&Cs);
}
void TCriticalSection::Leave() {
	pthread_mutex_unlock(&Cs);
}


TBlocker::TBlocker() {
	/*Event = CreateEvent(
        NULL,               // default security attributes
        false,               // auto-reset event
        false,              // initial state is nonsignaled
        NULL);	// object name
	IAssert(Event != NULL);*/
	pthread_cond_init(&Event, NULL);
	pthread_mutex_init(&Mutex, NULL);
}

TBlocker::~TBlocker() {
	pthread_cond_destroy(&Event);
	pthread_mutex_destroy(&Mutex);
}

void TBlocker::Block(int Msecs) {
	timespec waitTime;
	long nano = Msecs * 1000000;
	waitTime.tv_nsec = nano;
	pthread_cond_timedwait(&Event, &Mutex, &waitTime);
}
void TBlocker::Release() {
	pthread_cond_broadcast(&Event);
}

////////////////////////////////////////////
// Thread
const int TThread::STATUS_CREATED = 0;
const int TThread::STATUS_STARTED = 1;
const int TThread::STATUS_CANCELLED = 2;
const int TThread::STATUS_FINISHED = 3;

void * TThread::EntryPoint(void * pArg) {
    TThread *pThis = (TThread *)pArg;

    // set the routine which cleans up after the thread has finished
    pthread_cleanup_push(SetFinished, pThis);

    try {
    	pThis->Run();
    } catch (...) {
    	printf("Unknown exception while running thread: %lu!\n", pThis->GetThreadId());
    }

    // pop and execute the cleanup routine
    pthread_cleanup_pop(1);
    return 0;
}

void TThread::SetFinished(void *pArg) {
	TThread *pThis = (TThread *) pArg;

	TLock Lck(pThis->CriticalSection);
	pThis->Status = STATUS_FINISHED;
}

TThread::TThread():
		ThreadHandle(),
		CriticalSection(),
		Status(STATUS_CREATED) { }

TThread::~TThread() {
	TLock Lck(CriticalSection);
	if (IsAlive() && !IsCancelled()) {
		Cancel();
	}
}

TThread &TThread::operator =(const TThread& Other) {
	ThreadHandle = Other.ThreadHandle;
	CriticalSection = Other.CriticalSection;
	return *this;
}

void TThread::Start() {
	TLock Lck(CriticalSection);

	if (IsAlive()) {
		printf("Tried to start a thread that is already alive! Ignoring ...\n");
		return;
	}
	if (IsCancelled()) {
		return;
	}

	Status = STATUS_STARTED;

    // create new thread
	int code = pthread_create(
			&ThreadHandle,	// Handle
			NULL,			// Attributes
			EntryPoint,		// Thread func
			this			// Arg
	);
	EAssert(code == 0);
}

void TThread::Cancel() {
	TLock Lck(CriticalSection);

	if (!IsAlive()) { return; }
	Status = STATUS_CANCELLED;
	int code = pthread_cancel(ThreadHandle);
	EAssertR(code == 0, "Failed to cancel thread!");
}

int TThread::Join() {
	pthread_join(ThreadHandle, NULL);
	return 0;
}

bool TThread::IsAlive() {
	TLock Lck(CriticalSection);
	return Status == STATUS_STARTED || IsCancelled();
}

bool TThread::IsCancelled() {
	TLock Lck(CriticalSection);
	return Status == STATUS_CANCELLED;
}

bool TThread::IsFinished() {
	TLock Lck(CriticalSection);
	return Status == STATUS_FINISHED;
}

int TThread::GetCoreCount() {
	int NumCpu;
	#if defined(GLib_LINUX) || defined(GLIB_SOLARIS) || defined(GLib_CYGWIN)
		NumCpu = sysconf( _SC_NPROCESSORS_ONLN );
	#elif defined(GLib_BSD)
		nt mib[4];
		size_t len;

		/* set the mib for hw.ncpu */
		mib[0] = CTL_HW;
		mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

		/* get the number of CPUs from the system */
		sysctl(mib, 2, &NumCpu, &len, NULL, 0);

		if( numCPU < 1 )
		{
			 mib[1] = HW_NCPU;
			 sysctl( mib, 2, &NumCpu, &len, NULL, 0 );

			 if( NumCpu < 1 )
			 {
				  NumCpu = 1;
			 }
		}
	#else
		NumCpu = 1;
	#endif

	return NumCpu;
}
