#include <iostream>
#include <vector>
#include <chrono>
using namespace std;

int main()
{
    int n = 10000;
    int m = 10000;
    vector matrix(n, vector(m, 1));
    volatile int sum = 0;
    auto start = chrono::high_resolution_clock::now();
    int temp_sum = 0;
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < m; ++j)
        {
            temp_sum += matrix[i][j];
        }
    }
    sum = temp_sum;
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "Sum: " << sum << endl;
    cout << "Time: " << elapsed.count() << " seconds" << endl;
    return 0;
}