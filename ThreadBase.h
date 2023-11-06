/**
 * @file ThreadBase.h
 * @author nieyang (nieyang2003@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-06
 * 
 * 
 */
#pragma once

#include <thread>
#include <atomic>

/**
 * @brief 线程基类
 * 
 */
class ThreadBase
{
public:
    ThreadBase();

    virtual void run() = 0;

    void stop();

    void start();

private:
    std::thread *m_thread = nullptr;

protected:
    std::atomic<bool> m_stop = false;

};