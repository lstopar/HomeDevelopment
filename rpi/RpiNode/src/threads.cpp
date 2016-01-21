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
