#include "ThreadBase.h"

static void ThreadEntry(void *arg)
{
    ThreadBase * thread = (ThreadBase*)arg;
    thread->run();
}

ThreadBase::ThreadBase()
{
}

void ThreadBase::stop()
{
    m_stop = true;
    if (m_thread) {
        m_thread->join();
    }
}

void ThreadBase::start()
{
    m_stop = false;
    if (m_thread) {
        return ;
    } else {
        m_thread = new std::thread(ThreadEntry, this);
    }
}
