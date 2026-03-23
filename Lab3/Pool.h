
#ifndef POOL_H
#define POOL_H
#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <random>
#include <atomic>
using namespace std;
extern mutex console_mut;

class ThreadPool
{
private:
    vector<thread> m_workers;
    thread m_sleeper;
    queue<function<void()>> m_queue_todo;
    queue<function<void()>> m_queue_wait;
    mutex m_mutex;
    condition_variable m_cv_workers;
    condition_variable m_cv_sleeper;
    bool m_stop = false;
    bool m_imm_stop = false;
    bool m_paused = false;
    atomic<int> m_total_wait{0};
    atomic<int> m_wait_count{0};

    atomic<int> m_total_exec{0};
    atomic<int> m_exec_count{0};

    int m_sum_wait_len = 0;
    int m_wait_samples = 0;

    int m_sum_todo_len = 0;
    int m_todo_samples = 0;
    private:
        void Workers();
        void Sleeper();

    public:
        ThreadPool();
        ~ThreadPool();

        void AddTask(function<void()> task);
        void Pause();
        void Resume();
        void StopImm();
        void StopChill();
        void PrintStats();
};
#endif //POOL_H
