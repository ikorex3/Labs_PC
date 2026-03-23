#include "Pool.h"

void ThreadPool::Workers()
{
    while(true)
    {
        function<void()> task;
        {
            unique_lock<mutex> lock(m_mutex);
            auto wait_start = chrono::steady_clock::now();

            m_cv_workers.wait(lock,[this]()
            {
                return m_imm_stop || (m_stop && m_queue_todo.empty()) || (!m_queue_todo.empty() && !m_paused);
            });
            auto wait_end = chrono::steady_clock::now();
            m_total_wait += chrono::duration_cast<chrono::milliseconds>(wait_end - wait_start).count();
            m_wait_count++;
            if(m_imm_stop || (m_stop && m_queue_todo.empty()))
            {
                return;
            }
            m_sum_todo_len += m_queue_todo.size();
            m_todo_samples++;
            task = move(m_queue_todo.front());
            m_queue_todo.pop();
        }
        auto exec_start = chrono::steady_clock::now();
        task();
        auto exec_end = chrono::steady_clock::now();
        m_total_exec += chrono::duration_cast<chrono::milliseconds>(exec_end - exec_start).count();
        m_exec_count++;
    }
}

void ThreadPool::Sleeper()
{
    while(true)
    {
        unique_lock<mutex> lock(m_mutex);
        m_cv_sleeper.wait_for(lock, chrono::seconds(45), [this]()
        {
            return m_stop || m_imm_stop;
        });

        if(m_imm_stop || (m_stop && m_queue_wait.empty()))
        {
            return;
        }

        while(!m_queue_wait.empty())
        {
            m_queue_todo.push(move(m_queue_wait.front()));
            m_queue_wait.pop();
        }

        if(!m_queue_todo.empty())
        {
            m_cv_workers.notify_all();
        }
    }
}

ThreadPool::ThreadPool()
{
    for(int i = 0; i<4; ++i)
    {
        m_workers.emplace_back(&ThreadPool::Workers, this);
    }
    m_sleeper = thread(&ThreadPool::Sleeper, this);
}

ThreadPool::~ThreadPool()
{
    StopChill();
}

void ThreadPool::AddTask(function<void()> task)
{
    unique_lock<mutex> lock(m_mutex);
    if(m_stop || m_imm_stop) return;
    m_queue_wait.push(move(task));
    m_sum_wait_len += m_queue_wait.size();
    m_wait_samples++;
}

void ThreadPool::Pause()
{
    unique_lock<mutex> lock(m_mutex);
    m_paused = true;
}

void ThreadPool::Resume()
{
    {
        unique_lock<mutex> lock(m_mutex);
        m_paused = false;
    }
    m_cv_workers.notify_all();
}

void ThreadPool::StopImm()
{
    {
        unique_lock<mutex> lock(m_mutex);
        m_imm_stop = true;
        queue<function<void()>>().swap(m_queue_todo);
        queue<function<void()>>().swap(m_queue_wait);
    }
    m_cv_workers.notify_all();
    m_cv_sleeper.notify_all();
    for(thread& worker : m_workers)
    {
        if(worker.joinable()) worker.join();
    }
    if(m_sleeper.joinable()) m_sleeper.join();
}

void ThreadPool::StopChill()
{
    {
        unique_lock<mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv_workers.notify_all();
    m_cv_sleeper.notify_all();
    for(thread& worker : m_workers)
    {
        if(worker.joinable()) worker.join();
    }
    if(m_sleeper.joinable()) m_sleeper.join();
}

void ThreadPool::PrintStats()
{
    lock_guard<mutex> lock(console_mut);
    cout << "Count of threads in pool: " << m_workers.size() + 1 << endl;
    if (m_wait_count > 0)
    {
        cout << "Average time for thread to wait: " << m_total_wait/m_wait_count << "ms\n";
    }
    if(m_wait_samples > 0)
    {
        cout << "Average length for wait_queue: " << m_sum_wait_len / m_wait_samples << " tasks\n";
    }
    if (m_todo_samples > 0)
    {
        cout << "Average length for todo_queue: " << m_sum_todo_len / m_todo_samples << " tasks\n";
    }

    if (m_exec_count > 0)
    {
        cout << "Average time for doing a task: " << m_total_exec / m_exec_count << "ms\n";
    }
}