#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <iomanip>

using namespace std;

struct BankTransaction {
    string holderName;
    string cardNumber;
    chrono::system_clock::time_point date;
    string operation;
    double amount;
};

// Генерация случайных транзакций
vector<BankTransaction> generateTransactions(int count) {
    vector<BankTransaction> transactions;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> opDist(0, 3);
    uniform_int_distribution<> dateDist(1, 365);
    uniform_real_distribution<> amountDist(100, 10000);

    vector<string> operations = {"пополнение", "снятие", "перевод", "оплата"};

    for (int i = 0; i < count; ++i) {
        transactions.push_back({
            "Holder " + to_string(i),
            "1234-5678-9012-" + to_string(i),
            chrono::system_clock::now() - chrono::hours(24 * dateDist(gen)),
            operations[opDist(gen)],
            amountDist(gen)
        });
    }

    return transactions;
}

// Однопоточная обработка
double singleThreadProcessing(const vector<BankTransaction>& transactions, const string& operation,
                              const chrono::system_clock::time_point& startDate,
                              const chrono::system_clock::time_point& endDate) {
    return accumulate(transactions.begin(), transactions.end(), 0.0,
                      [&](double sum, const BankTransaction& t) {
                          return sum + (t.operation == operation && t.date >= startDate && t.date <= endDate ? t.amount : 0);
                      });
}

// Многопоточная обработка
double multiThreadProcessing(const vector<BankTransaction>& transactions, const string& operation,
                             const chrono::system_clock::time_point& startDate,
                             const chrono::system_clock::time_point& endDate) {
    int threadCount = thread::hardware_concurrency();
    vector<thread> threads;
    vector<double> results(threadCount, 0.0);

    int chunkSize = transactions.size() / threadCount;

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i]() {
            int start = i * chunkSize;
            int end = (i == threadCount - 1) ? transactions.size() : (i + 1) * chunkSize;
            for (int j = start; j < end; ++j) {
                if (transactions[j].operation == operation && transactions[j].date >= startDate && transactions[j].date <= endDate) {
                    results[i] += transactions[j].amount;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return accumulate(results.begin(), results.end(), 0.0);
}

// Функция для измерения времени выполнения
template<typename Func>
void measure_time(const string& name, Func func) {
    auto start = chrono::high_resolution_clock::now();
    double result = func();
    auto end = chrono::high_resolution_clock::now();
    cout << setw(20) << left << name << ": " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms, Result: " << result << "\n";
}

int main() {
    auto transactions = generateTransactions(1000000);
    string operation = "пополнение";
    auto startDate = chrono::system_clock::now() - chrono::hours(24 * 30);
    auto endDate = chrono::system_clock::now();

    // Однопоточная обработка
    measure_time("Single Thread", [&]() {
        return singleThreadProcessing(transactions, operation, startDate, endDate);
    });

    // Многопоточная обработка
    measure_time("Multi Thread", [&]() {
        return multiThreadProcessing(transactions, operation, startDate, endDate);
    });

    return 0;
}