#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <iomanip>
#include <string>

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
                             const chrono::system_clock::time_point& endDate, int threadCount) {
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

// Функция для вывода информации о транзакциях
void printTransactions(const vector<BankTransaction>& transactions, const string& operation,
                       const chrono::system_clock::time_point& startDate,
                       const chrono::system_clock::time_point& endDate) {
    cout << setw(20) << left << "Holder Name"
         << setw(20) << left << "Card Number"
         << setw(20) << left << "Date"
         << setw(20) << left << "Operation"
         << setw(10) << left << "Amount" << "\n";

    for (const auto& t : transactions) {
        if (t.operation == operation && t.date >= startDate && t.date <= endDate) {
            time_t time = chrono::system_clock::to_time_t(t.date);
            cout << setw(20) << left << t.holderName
                 << setw(20) << left << t.cardNumber
                 << setw(20) << left << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S")
                 << setw(20) << left << t.operation
                 << setw(10) << left << t.amount << "\n";
        }
    }
}

// Функция для измерения времени выполнения
template<typename Func>
void measure_time(const string& name, Func func) {
    auto start = chrono::high_resolution_clock::now();
    double result = func();
    auto end = chrono::high_resolution_clock::now();
    cout << setw(20) << left << name << ": " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms, Result: " << result << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0] << " <transaction_count> <thread_count> <operation> <start_date_days_ago> <end_date_days_ago>\n";
        return 1;
    }

    int transactionCount = stoi(argv[1]);
    int threadCount = stoi(argv[2]);
    string operation = argv[3];
    int startDateDaysAgo = stoi(argv[4]);
    int endDateDaysAgo = stoi(argv[5]);

    auto transactions = generateTransactions(transactionCount);
    auto startDate = chrono::system_clock::now() - chrono::hours(24 * startDateDaysAgo);
    auto endDate = chrono::system_clock::now() - chrono::hours(24 * endDateDaysAgo);

    // Вывод информации о транзакциях
    printTransactions(transactions, operation, startDate, endDate);

    // Однопоточная обработка
    double singleThreadResult = 0.0;
    measure_time("Single Thread", [&]() {
        singleThreadResult = singleThreadProcessing(transactions, operation, startDate, endDate);
        return singleThreadResult;
    });
    cout << "Single Thread Result: " << singleThreadResult << "\n";

    // Многопоточная обработка
    double multiThreadResult = 0.0;
    measure_time("Multi Thread", [&]() {
        multiThreadResult = multiThreadProcessing(transactions, operation, startDate, endDate, threadCount);
        return multiThreadResult;
    });
    cout << "Multi Thread Result: " << multiThreadResult << "\n";

    return 0;
}
