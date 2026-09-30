#ifndef OSAPI_RECURSIVE_MUTEX_WINDOWS_H
#define OSAPI_RECURSIVE_MUTEX_WINDOWS_H

namespace osapi {

class RecursiveMutex : public MutexInterface
{
public:
    RecursiveMutex() {
        mMutex = CreateMutex(nullptr, false, nullptr);
    }
    
    virtual ~RecursiveMutex() {
        if (mMutex) {
            CloseHandle(mMutex);
        }
    }

    virtual bool lock(unsigned int timeout) {
        if (mMutex) {
            DWORD taken = WaitForSingleObject(mMutex, timeout);
            if(WAIT_OBJECT_0 == taken) {
                return true;
            }
        }
        return false;
    }

    virtual void unlock() {
        if (mMutex) {
            ReleaseMutex(mMutex);
        }
    }

    private:
        HANDLE mMutex;
};

} // namespace osapi

#endif // OSAPI_RECURSIVE_MUTEX_WINDOWS_H
