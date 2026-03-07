#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
using namespace std;
int main() {
    int n = 1000;
    random_device rd;
    default_random_engine generator(rd());
    uniform_int_distribution<int> distribution(0,1000000);
    vector<int> arr(n);
    for(int i = 0; i<n; i++) {
        arr[i] = distribution(generator);
    }
    auto start = chrono::high_resolution_clock::now();
    int count = 0;
    int max = 0;
    for(int a:arr) {
        if(a%2==0) {
            count++;
            if(max<a) {
                max = a;
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start);
    cout<<"Count: " << count << endl;
    cout<<"Max even: " << max << endl;
    cout << "Time: " << fixed << setprecision(9) << elapsed.count() * 1e-9 << " seconds" << endl;
    return 0;
}