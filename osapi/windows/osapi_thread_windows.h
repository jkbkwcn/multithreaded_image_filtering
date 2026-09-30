#ifndef OSAPI_THREAD_WINDOWS_H
#define OSAPI_THREAD_WINDOWS_H

#include "windows.h"
#include <stdlib.h>
#include "windows_proc_info.h"

namespace osapi {

/** Thread interface implementation for Windows. */
class Thread : public ThreadInterface
{
    public:

        /** Thread constructor.
         *  @param[in] priority thread priority
         *  @param[in] stackSize thread stack size in bytes
         *  @param[in] isJoinable decides if the thread supports join operation or not
         *  @param[in] name optional thread name
         */
        Thread(int priority, unsigned int stackSize, Joinable isJoinable, const char* name = "unnamed") : mPriority{priority}, mStackSize{stackSize}, mIsJoinable{isJoinable}, mName{name}, mThreadHandle{nullptr} {}
        
        /** Virtual destructor required to properly destroy derived class objects. */
        virtual ~Thread() {
            if(mThreadHandle) {
                CloseHandle(mThreadHandle);
            }
        }
        
        /** Runs the thread.
         *  @retval true if the thread was started successfully,
         *  @retval false if the thread was not started successfully, or the thread was already running
         */
        virtual bool run() {
            if(isRunning()) {
        		return false;
        	}
			mThreadHandle = CreateThread(nullptr, mStackSize, (LPTHREAD_START_ROUTINE)threadFunction, this, 0, nullptr);
            if(nullptr != mThreadHandle) {
                setPriority(mPriority);
                return true;
            }
			return false;
        }
        
        /** Checks if the thread is running.
         *  @retval true if the thread is running
         *  @retval false if the thread is not running
         */
        virtual bool isRunning() {
            return (nullptr != mThreadHandle);
        }

        /** Waits for the thread to finish executing, with a given timeout.
         *  @param timeout[in] number of milliseconds to wait for the thread to finish executing
         *  @retval true if the thread was successfully joined in the given time
         *  @retval false if the thread was not joined within the given time or the thread is not joinable at all
         */
        virtual bool join(unsigned int timeout) {
            if(false == mIsJoinable) {
                return false;
            }
            if(nullptr == mThreadHandle) {
                return false;
            }
            DWORD joined = WaitForSingleObject(mThreadHandle, timeout);
            if(WAIT_OBJECT_0 == joined) {
                CloseHandle(mThreadHandle);
                mThreadHandle = nullptr;
                return true;
            }
            return false;
        }

        /** Checks, if the thread is joinable.
         *  @retval true if the thread is joinable
         *  @retval false if the thread is not joinable
         */
        virtual bool isJoinable() {
            return mIsJoinable;
        }

        /** Suspends thread execution.
         *  @retval true if the thread was suspended successfully
         *  @retval false if the thread was not suspended for some reason
         */
        virtual bool suspend() {
            if(nullptr == mThreadHandle) {
        		return false;
        	}
			DWORD suspended = SuspendThread(mThreadHandle);
			return ((DWORD)-1 != suspended);
        }

        /** Resumes thread execution.
         *  @retval true if the thread was resumed successfully
         *  @retval false if the thread was not resumed for some reason
         */
        virtual bool resume() {
            if(nullptr == mThreadHandle) {
        		return false;
        	}
            DWORD resumed = ResumeThread(mThreadHandle);
            return ((DWORD)-1 != resumed);
        }

        /** Sets thread priority
         *  @param[in] priority new thread priority
         *  @retval true if the priority for the thread was set successfully
         *  @retval false if the priority for the thread was not set successfully for some reason
         */
        virtual bool setPriority(int priority) {
            mPriority = priority;
            if(nullptr != mThreadHandle) {
        		return (SetThreadPriority(mThreadHandle, priority));
        	}
            return true;
        }

        /** Gets the thread priority
         *  @return current thread priority
         */
        virtual int getPriority() {
            if(nullptr == mThreadHandle) {
				return mPriority;
			}
            return GetThreadPriority(mThreadHandle);
        }
        
        /** Gets thread name
         *  @return name of the thread
         */
        virtual const char* getName() {
            return mName;
        }

        /** Gets the current state of the thread
         *  @return current state of the thread
         */
        virtual ThreadState getState() {
            if(nullptr == mThreadHandle) {
        		return INACTIVE;
        	}
            
            DWORD u32_Error = mProcInfo.Capture();
            // Error capturing system info
            if (u32_Error)
            {
                return UNKNOWN;
            }

            SYSTEM_PROCESS_INFORMATION* processInformation = mProcInfo.FindProcessByPid(GetCurrentProcessId());
            // Error while searching for process info
            if (!processInformation)
            {
                return UNKNOWN;
            }

            SYSTEM_THREAD_INFORMATION* threadInformation = mProcInfo.FindThreadByTid(processInformation, GetThreadId(mThreadHandle));
            // Error while searching for thread info
            if (!threadInformation)
            {
                return UNKNOWN;
            }

            switch (threadInformation->ThreadState) {
            case StateInitialized:
                return INACTIVE;
                break;
            case StateReady:
                return READY;
                break;
            case StateRunning:
                return RUNNING;
                break;
            case StateStandby:
                return READY;
                break;
            case StateTerminated:
                return INACTIVE;
                break;
            case StateTransition:
                return BLOCKED;
                break;
            case StateUnknown:
                return UNKNOWN;
                break;
            case StateWait:
                BOOL b_Suspend;
                mProcInfo.IsThreadSuspended(threadInformation, &b_Suspend);
                if(b_Suspend) {
                    return SUSPENDED;
                }
                return BLOCKED;
                break;
            }
            return UNKNOWN;
        }

        /** Gets the total size of the stack assigned for this thread
         *  @return total stack size in number of bytes
         */
        virtual unsigned int getTotalStackSize() {
            if(nullptr == mThreadHandle) {
                return mStackSize;
            }
            return mProcInfo.getTotalStackSize(mThreadHandle);
        }

        /** Gets the size of the used stack for this thread
         *  @return number of bytes used on the stack
         */
        virtual unsigned int getUsedStackSize() {
            if(nullptr == mThreadHandle) {
                return 0;
            }
            return mProcInfo.getUsedStackSize(mThreadHandle);
        }

    protected:

        /** Delays thread execution for a given time.
         *  @param time[in] number of milliseconds to delay thread execution
         */
        virtual void sleep(unsigned int time) {
            Sleep(time);
        }

    private:
		int mPriority;
        unsigned int mStackSize;
        Joinable mIsJoinable;
        const char* mName;
        HANDLE mThreadHandle;
        WindowsProcInfo mProcInfo;

        static DWORD WINAPI threadFunction(void* argument) {
        	Thread* osapiThreadObject = reinterpret_cast<Thread*>(argument);
        	if (osapiThreadObject) {
        		osapiThreadObject->job();
        	}
            return 0;
        }
};

} // namespace osapi

#endif // OSAPI_THREAD_WINDOWS_H
