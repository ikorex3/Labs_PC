#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>
#include <atomic>
using namespace std;

void Task(const vector<int>& arr, int start, int end, atomic<int>& count, atomic<int>& max){
    int local_count = 0;
    int local_max = 0;
    for(int i = start; i < end; i++) {
        if(arr[i]%2==0) {
            local_count++;
            if(local_max<arr[i]) {
                local_max = arr[i];
            }
        }
    }
    int current_count = count.load();
    while(!count.compare_exchange_weak(current_count, current_count+local_count))
    {}
    int current_max = max.load();
    while(local_max>current_max) {
        if(max.compare_exchange_weak(current_max, local_max)) {
            break;
        }
    }
}

int main() {
    int threads_count = 8;
    int n = 100000000;
    atomic<int> final_count = 0;
    atomic<int> final_max = 0;
    random_device rd;
    default_random_engine generator(rd());
    uniform_int_distribution<int> distribution(0,1000000);
    vector<int> arr(n);
    for(int i = 0; i<n; i++) {
        arr[i] = distribution(generator);
    }
    vector<thread> threads;
    auto start_time = chrono::high_resolution_clock::now();
    int step = n/threads_count;
    for(int i = 0; i<threads_count;i++) {
        int start = i * step;
        int end = start + step;
        if (i == threads_count - 1)
        {
            end = n;
        }
        threads.emplace_back(Task, ref(arr), start, end, ref(final_count), ref(final_max));
    }
    for (auto& t:threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end_time - start_time);
    cout<<"Count: " << final_count << endl;
    cout<<"Max even: " << final_max << endl;
    cout << "Time: " << fixed << setprecision(9) << elapsed.count() * 1e-9 << " seconds" << endl;
    return 0;
}