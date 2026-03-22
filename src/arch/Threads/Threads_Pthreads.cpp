#include "global.h"
#include "Threads_Pthreads.h"
#include "RageUtil.h"
#include <chrono>
#include <cerrno>
#include <cstring>

#if defined(LINUX)
#include "archutils/Unix/LinuxThreadHelpers.h"
#include "archutils/Unix/RunningUnderValgrind.h"
#endif

#if defined(DARWIN)
#include "archutils/Darwin/DarwinThreadHelpers.h"
#endif

void ThreadImpl_Pthreads::Halt( bool Kill )
{
	/* Linux:
	 * Send a SIGSTOP to the thread.  If we send a SIGKILL, pthreads will
	 * "helpfully" propagate it to the other threads, and we'll get killed, too.
	 *
	 * This isn't ideal, since it can cause the process to background as far as
	 * the shell is concerned, so the shell prompt can display before the crash
	 * handler actually displays a message.
	 */
	SuspendThread( threadHandle );
}

void ThreadImpl_Pthreads::Resume()
{
	/* Linux: Send a SIGCONT to the thread. */
	ResumeThread( threadHandle );
}

uint64_t ThreadImpl_Pthreads::GetThreadId() const
{
	return threadHandle;
}

int ThreadImpl_Pthreads::Wait()
{
	void *val;
	int ret = pthread_join( thread, &val );
	if( ret )
		RageException::Throw( "pthread_join: %s", strerror(errno) );

	return static_cast<int>(reinterpret_cast<intptr_t>(val));
}

ThreadImpl *MakeThisThread()
{
	ThreadImpl_Pthreads *thread = new ThreadImpl_Pthreads;

	thread->thread = pthread_self();
	thread->threadHandle = GetCurrentThreadId();

	return thread;
}

static void *StartThread( void *pData )
{
	ThreadImpl_Pthreads *pThis = static_cast<ThreadImpl_Pthreads *>(pData);

	pThis->threadHandle = GetCurrentThreadId();
	*pThis->m_piThreadID = pThis->threadHandle;

	/* Tell MakeThread that we've set m_piThreadID, so it's safe to return. */
	pThis->m_StartFinishedSem->Post();

	return reinterpret_cast<void *>(pThis->m_pFunc( pThis->m_pData ));
}

ThreadImpl *MakeThread( int (*pFunc)(void *pData), void *pData, uint64_t *piThreadID )
{
	ThreadImpl_Pthreads *thread = new ThreadImpl_Pthreads;
	thread->m_pFunc = pFunc;
	thread->m_pData = pData;
	thread->m_piThreadID = piThreadID;

	thread->m_StartFinishedSem = new SemaImpl_Pthreads( 0 );

	int ret = pthread_create( &thread->thread, nullptr, StartThread, thread );
	if( ret )
		FAIL_M( ssprintf( "MakeThread: pthread_create: %s", strerror(errno)) );

	/* Don't return until StartThread sets m_piThreadID. */
	thread->m_StartFinishedSem->Wait();
	delete thread->m_StartFinishedSem;

	return thread;
}

/* Priority 5.2: MutexImpl_Pthreads now uses std::timed_mutex instead of pthread_mutex_t.
 * Deadlock detection uses try_lock_for() with a 10-second timeout, equivalent to
 * the previous pthread_mutex_timedlock() approach. */
MutexImpl_Pthreads::MutexImpl_Pthreads( RageMutex *pParent ):
	MutexImpl( pParent )
{
	/* std::timed_mutex is default-constructed, no initialization needed */
}

MutexImpl_Pthreads::~MutexImpl_Pthreads()
{
	/* std::timed_mutex destructor handles cleanup */
}

bool MutexImpl_Pthreads::Lock()
{
#if defined(LINUX)
	/* Valgrind can have issues with timed locks; use a plain lock instead. */
	if( RunningUnderValgrind() )
	{
		mutex.lock();
		return true;
	}
#endif

	/* Wait up to 10 seconds. If it takes longer, we're probably deadlocked. */
	if( mutex.try_lock_for( std::chrono::seconds(10) ) )
		return true;

	/* Timed out once — try one more second (in case we paused in a debugger). */
	if( mutex.try_lock_for( std::chrono::seconds(1) ) )
		return true;

	/* Still can't acquire: deadlock detected. Return false so caller can report it. */
	return false;
}

bool MutexImpl_Pthreads::TryLock()
{
	return mutex.try_lock();
}

void MutexImpl_Pthreads::Unlock()
{
	mutex.unlock();
}

uint64_t GetThisThreadId()
{
	return GetCurrentThreadId();
}

uint64_t GetInvalidThreadId()
{
	return 0;
}

MutexImpl *MakeMutex( RageMutex *pParent )
{
	return new MutexImpl_Pthreads( pParent );
}

/* Priority 5.4: SemaImpl_Pthreads now uses std::condition_variable + std::mutex
 * instead of raw pthread_cond_t + pthread_mutex_t.  This preserves the custom
 * counting semaphore semantics needed for correct behavior on all platforms
 * (including macOS, which historically had broken POSIX semaphores). */
SemaImpl_Pthreads::SemaImpl_Pthreads( int iInitialValue )
	: m_iValue( static_cast<unsigned>(iInitialValue) )
{
}

SemaImpl_Pthreads::~SemaImpl_Pthreads()
{
	/* std::condition_variable and std::mutex destructors handle cleanup */
}

void SemaImpl_Pthreads::Post()
{
	{
		std::lock_guard<std::mutex> lock( m_Mutex );
		++m_iValue;
	}
	m_Cond.notify_one();
}

bool SemaImpl_Pthreads::Wait()
{
	std::unique_lock<std::mutex> lock( m_Mutex );

	/* Wait up to 10 seconds. If it takes longer, we're probably deadlocked. */
	bool signaled = m_Cond.wait_for( lock, std::chrono::seconds(10),
		[this]{ return m_iValue > 0; } );

	if( !signaled )
	{
		/* Timed out once — try one more second (in case we paused in a debugger). */
		signaled = m_Cond.wait_for( lock, std::chrono::seconds(1),
			[this]{ return m_iValue > 0; } );
	}

	if( !signaled )
		return false;

	--m_iValue;
	return true;
}

bool SemaImpl_Pthreads::TryWait()
{
	std::lock_guard<std::mutex> lock( m_Mutex );
	if( !m_iValue )
		return false;

	--m_iValue;
	return true;
}

SemaImpl *MakeSemaphore( int iInitialValue )
{
	return new SemaImpl_Pthreads( iInitialValue );
}


/*
 * (c) 2001-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
