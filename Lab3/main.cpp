#include <iostream>
#include <random>
#include "Pool.h"
using namespace std;
mutex console_mut;

void Producer(int producer_id, ThreadPool& pool, bool& keep_producing)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> task_duration_dist(4, 10);
    uniform_int_distribution<> sleep_dist(1, 3);
    int task_count = 1;
    while (keep_producing)
    {
        int duration = task_duration_dist(gen);
        int task_id = producer_id * 100 + task_count;
        pool.AddTask([task_id, duration]()
        {
            {
                lock_guard<mutex> lock(console_mut);
                cout << "- Task " << task_id << " STARTED (duration: " << duration << "s), thread " << this_thread::get_id() << "\n";
            }
            this_thread::sleep_for(chrono::seconds(duration));
            {
                lock_guard<mutex> lock(console_mut);
                cout << "+ Task " << task_id << " COMPLETED\n";
            }
        });
        {
            lock_guard<mutex> lock(console_mut);
            cout << producer_id << " added task " << task_id << " to wait\n";
        }
        task_count++;
        this_thread::sleep_for(chrono::seconds(sleep_dist(gen)));
    }
}

int main()
{
    ThreadPool pool;
    bool keep_producing = true;
    vector<thread> producers;
    for (int i = 1; i <= 3; ++i)
    {
        producers.emplace_back(Producer, i, ref(pool), ref(keep_producing));
    }
    this_thread::sleep_for(chrono::seconds(60));
    cout << "\n!!! MAIN 60 seconds passed. Stopping producing. !!! \n";
    keep_producing = false;
    for (auto& p : producers) {
        if (p.joinable()) {
            p.join();
        }
    }
    cout << "\nMain producers stopped\n";
    pool.StopChill();
    cout << "Main finished\n";

    pool.PrintStats();
    return 0;
}