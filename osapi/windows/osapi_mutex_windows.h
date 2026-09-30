#ifndef OSAPI_MUTEX_WINDOWS_H
#define OSAPI_MUTEX_WINDOWS_H

namespace osapi {

class Mutex : public MutexInterface
{
public:
    Mutex() {
        mMutex = CreateSemaphore(nullptr, 1, 1, nullptr);
    }

    virtual ~Mutex() {
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
            ReleaseSemaphore(mMutex, 1, nullptr);
        }
    }

    private:
        HANDLE mMutex;
};

} // namespace osapi

#endif // OSAPI_MUTEX_WINDOWS_H
