#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <map>
#include <algorithm>

using namespace std;

const int NUM_THREADS = 10;
const int NUM_ITERATIONS = 1000;

// Mutex
mutex mtx;
void generateRandomCharMutex(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        lock_guard<mutex> lock(mtx);  // Захватываем мьютекс
        // Не выводим символы в терминал
    }
}

// Semaphore
class Semaphore {
public:
    Semaphore(int count = 0) : count(count) {}

    void acquire() {
        unique_lock<mutex> lock(mtx);  // Захватываем мьютекс
        cv.wait(lock, [this]() { return count > 0; });  // Ожидаем, пока счетчик будет больше 0
        --count;  // Уменьшаем счетчик
    }

    void release() {
        lock_guard<mutex> lock(mtx);  // Захватываем мьютекс
        ++count;  // Увеличиваем счетчик
        cv.notify_one();  // Уведомляем один из ожидающих потоков
    }

private:
    mutex mtx;
    condition_variable cv;
    int count;
};

Semaphore sem(1);
void generateRandomCharSemaphore(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        sem.acquire();  // Захватываем семафор
        // Не выводим символы в терминал
        sem.release();  // Освобождаем семафор
    }
}

// SemaphoreSlim
Semaphore semSlim(NUM_THREADS);
void generateRandomCharSemaphoreSlim(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        semSlim.acquire();  // Захватываем семафор
        // Не выводим символы в терминал
        semSlim.release();  // Освобождаем семафор
    }
}

// Barrier
class Barrier {
public:
    Barrier(int count) : threshold(count), count(count), generation(0) {}

    void arrive_and_wait() {
        unique_lock<mutex> lock(mtx);  // Захватываем мьютекс
        auto gen = generation;
        if (--count == 0) {  // Если все потоки прибыли
            generation++;  // Переходим к следующему поколению
            count = threshold;  // Сбрасываем счетчик
            cv.notify_all();  // Уведомляем все ожидающие потоки
        } else {
            cv.wait(lock, [this, gen]() { return gen != generation; });  // Ожидаем, пока не изменится поколение
        }
    }

private:
    mutex mtx;
    condition_variable cv;
    int threshold;
    int count;
    int generation;
};

Barrier bar(NUM_THREADS);
void generateRandomCharBarrier(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        bar.arrive_and_wait();  // Прибываем и ждем остальных потоков
        // Не выводим символы в терминал
    }
}

// SpinLock
atomic_flag spinLock = ATOMIC_FLAG_INIT;
void generateRandomCharSpinLock(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        while (spinLock.test_and_set(memory_order_acquire)) {}  // Цикл ожидания захвата spinlock
        // Не выводим символы в терминал
        spinLock.clear(memory_order_release);  // Освобождаем spinlock
    }
}

// SpinWait
atomic<bool> spinWaitFlag = false;
void generateRandomCharSpinWait(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        while (spinWaitFlag.exchange(true, memory_order_acquire)) {  // Цикл ожидания с возможностью уступить процессор
            this_thread::yield();
        }
        // Не выводим символы в терминал
        spinWaitFlag.store(false, memory_order_release);  // Освобождаем флаг
    }
}

// Monitor
mutex mtxMonitor;
condition_variable cvMonitor;
bool ready = false;
void generateRandomCharMonitor(int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        char c = static_cast<char>(dis(gen));
        unique_lock<mutex> lock(mtxMonitor);  // Захватываем мьютекс
        cvMonitor.wait(lock, []{ return ready; });  // Ожидаем, пока флаг ready не станет true
        // Не выводим символы в терминал
        ready = false;  // Сбрасываем флаг
        cvMonitor.notify_all();  // Уведомляем все ожидающие потоки
    }
}

// Функция для измерения времени выполнения
void measureTime(void (*func)(int), int threadId, map<string, double>& results, const string& name) {
    auto start = chrono::high_resolution_clock::now();
    func(threadId);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    results[name] = duration.count();
    cout << name << " Time taken: " << duration.count() << " seconds" << endl;
}

int main() {
    vector<thread> threads;
    map<string, double> results;

    cout << "Mutex:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharMutex, i, ref(results), "Mutex");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    cout << "Semaphore:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharSemaphore, i, ref(results), "Semaphore");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    cout << "SemaphoreSlim:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharSemaphoreSlim, i, ref(results), "SemaphoreSlim");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    cout << "Barrier:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharBarrier, i, ref(results), "Barrier");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    cout << "SpinLock:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharSpinLock, i, ref(results), "SpinLock");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    cout << "SpinWait:" << endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(measureTime, generateRandomCharSpinWait, i, ref(results), "SpinWait");
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    // Вывод результатов сравнения
    cout << "\nComparison of synchronization primitives:" << endl;
    vector<pair<string, double>> sortedResults(results.begin(), results.end());
    sort(sortedResults.begin(), sortedResults.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    for (const auto& result : sortedResults) {
        cout << result.first << ": " << result.second << " seconds" << endl;
    }

    return 0;
}