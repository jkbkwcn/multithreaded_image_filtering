#ifndef OSAPI_THREAD_INTERFACE_H
#define OSAPI_THREAD_INTERFACE_H

namespace osapi {

/** Enumeration describing two types of thread: joinable and non-joinable */
typedef enum {
    NOT_JOINABLE = 0,
    JOINABLE = 1
} Joinable;

/** Enumeration describing possible thread states */
typedef enum {
    UNKNOWN = 0,
    INACTIVE = 1,
    READY = 2,
    RUNNING = 3,
    BLOCKED = 4,
    SUSPENDED = 5
} ThreadState;

/** Base interface for all threads. */
class ThreadInterface
{
    public:
    
        /** Virtual destructor required to properly destroy derived class objects. */
        virtual ~ThreadInterface() { }

        /** Runs the thread.
        *  @retval true if the thread was started successfully, 
        *  @retval false if the thread was not started successfully, or the thread was already running
        */
        virtual bool run() = 0;
        
        /** Checks if the thread is running, meaning that it was run using the run() and has not yet finished running.
        *  @retval true if the thread is running
        *  @retval false if the thread is not running
        */
        virtual bool isRunning() = 0;
        
        /** Waits for the thread to finish executing, with a given timeout.
         *  @param timeout[in] number of milliseconds to wait for the thread to finish executing
         *  @retval true if the thread was successfully joined in the given time
         *  @retval false if the thread was not joined within the given time or the thread is not joinable at all
         */
        virtual bool join(unsigned int timeout) = 0;

        /** Checks, if the thread is joinable.
         *  @retval true if the thread is joinable
         *  @retval false if the thread is not joinable
         */
        virtual bool isJoinable() = 0;

        /** Suspends thread execution.
         *  @retval true if the thread was suspended successfully
         *  @retval false if the thread was not suspended for some reason
         */
        virtual bool suspend() = 0;

        /** Resumes thread execution.
         *  @retval true if the thread was resumed successfully
         *  @retval false if the thread was not resumed for some reason
         */
        virtual bool resume() = 0;

        /** Sets thread priority
         *  @param[in] priority new thread priority
         *  @retval true if the priority for the thread was set successfully
         *  @retval false if the priority for the thread was not set successfully for some reason
         */
        virtual bool setPriority(int priority) = 0;

        /** Gets the thread priority
         *  @return current thread priority
         */
        virtual int getPriority() = 0;

        /** Gets thread name
         *  @return name of the thread
         */
        virtual const char* getName() = 0;

        /** Gets the current state of the thread
         *  @return current state of the thread
         */
        virtual ThreadState getState() = 0;             

        /** Gets the total size of the stack assigned for this thread
         *  @return total stack size in number of bytes
         */
        virtual unsigned int getTotalStackSize() = 0;

        /** Gets the size of the used stack for this thread
         *  @return number of bytes used on the stack
         */
        virtual unsigned int getUsedStackSize() = 0;

    protected:

        /** Delays thread execution for a given time.
         *  @param time[in] number of milliseconds to delay thread execution
         */
        virtual void sleep(unsigned int time) = 0;

        /** Body of the thread - user defined thread function implementation. */
        virtual void job() = 0;

};

} // namespace osapi

#endif // OSAPI_THREAD_INTERFACE_H
