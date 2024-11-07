#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace std;

const int NUM_PHILOSOPHERS = 5;
mutex forks[NUM_PHILOSOPHERS];

void philosopher(int id) {
    int left_fork = id;
    int right_fork = (id + 1) % NUM_PHILOSOPHERS;

    while (true) {
        // Философ думает
        cout << "Philosopher " << id << " is thinking.\n";
        this_thread::sleep_for(chrono::milliseconds(1000));

        if (id == NUM_PHILOSOPHERS - 1) {
            // Последний философ берет вилки в обратном порядке
            unique_lock<mutex> right(forks[right_fork]);
            unique_lock<mutex> left(forks[left_fork], defer_lock);
            left.lock();
            // Философ ест
            cout << "Philosopher " << id << " is eating.\n";
            this_thread::sleep_for(chrono::milliseconds(1000));
            left.unlock();
            right.unlock();
        } else {
            // Остальные философы берут вилки в обычном порядке
            unique_lock<mutex> left(forks[left_fork]);
            unique_lock<mutex> right(forks[right_fork], defer_lock);
            right.lock();
            // Философ ест
            cout << "Philosopher " << id << " is eating.\n";
            this_thread::sleep_for(chrono::milliseconds(1000));
            right.unlock();
            left.unlock();
        }
    }
}

int main() {
    vector<thread> philosophers;

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers.emplace_back(philosopher, i);
    }

    for (auto& p : philosophers) {
        p.join();
    }

    return 0;
}
