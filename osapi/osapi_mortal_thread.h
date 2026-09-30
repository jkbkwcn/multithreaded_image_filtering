// 
// 
// TODO: replace this with your own implementation !!!
// 
// This is just a mock
//
//

#ifndef OSAPI_MORTAL_THREAD_H
#define OSAPI_MORTAL_THREAD_H

#include <csignal>
 
namespace osapi {

class MortalThread : public Thread
{
    public:
        MortalThread(int priority, unsigned int stackSize) : Thread(priority, stackSize, JOINABLE), killSignal{0} { ; }

        virtual ~MortalThread() { ; }

        void kill() { 
            killSignal = 1;
        }
        
    private:
        sig_atomic_t killSignal;

        /** Implementation of the job method */
        virtual void job(void) {
        begin();
        while(!killSignal) {
            loop();
        }
        end();
        }

    protected:
        virtual void begin() = 0;
        virtual void loop() = 0;
        virtual void end() = 0;
};

} // namespace osapi
 
#endif // OSAPI_MORTAL_THREAD_H