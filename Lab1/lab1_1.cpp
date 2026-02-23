#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
using namespace std;

void Summa(const vector<vector<int>>& matrix, int start, int end, int& sum)
{
    int temp_sum = 0;
    for (int i = start; i < end; ++i)
    {
        for (int j = 0; j < matrix[i].size(); ++j)
        {
            temp_sum += matrix[i][j];
        }
    }
    sum = temp_sum;
}

int main()
{
    vector m_size = {1000, 2000, 5000, 10000, 20000};
    vector thread_count = {2, 4, 8, 16, 32, 64, 128};
    for (int size : m_size)
    {
        int n = size;
        int m = size;
        cout << "\nMatrix: " << n << "x" << m << endl;

        for (int num_threads:thread_count)
        {
            vector matrix(n, vector(m,1));
            vector<thread> threads;
            vector results(num_threads,0);
            auto start = chrono::high_resolution_clock::now();
            int step = n / num_threads;
            for (int i = 0; i < num_threads; ++i)
            {
                int starts = i * step;
                int end = starts + step;
                if (i == num_threads - 1)
                {
                    end = n;
                }
                threads.emplace_back(Summa, ref(matrix), starts, end, ref(results[i]));
            }
            for (auto& t:threads)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
            volatile int total_sum = 0;
            for (int res:results)
            {
                total_sum += res;
            }
            auto end = chrono::high_resolution_clock::now();
            auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start);
            cout << "Threads: " << num_threads << " Time: " << elapsed.count()* 1e-9 << " seconds" << endl;
        }
    }
    return 0;
}