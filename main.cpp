#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std;

struct Config {
    int n{};
    int t{};
    int h{};
    int d{};
    int t1{};
    int t2{};
};

struct DungeonInstance {
    int id{};
    bool active{false};
    int currentPartyId{0};
    int partiesServed{0};
    int totalTimeServed{0};
};

string trim(const string& text) {
    const auto first = find_if_not(text.begin(), text.end(),
                                        [](unsigned char ch) { return isspace(ch); });
    if (first == text.end()) {
        return {};
    }
    const auto last = find_if_not(text.rbegin(), text.rend(),
                                       [](unsigned char ch) { return isspace(ch); }).base();
    return string(first, last);
}

int parseInteger(const string& value, const string& key, int lineNumber) {
    if (value.empty()) {
        throw runtime_error("Missing integer value for '" + key + "' on line " +
                                 to_string(lineNumber) + ".");
    }

    size_t used = 0;
    long long parsed = 0;
    try {
        parsed = stoll(value, &used, 10);
    } catch (const exception&) {
        throw runtime_error("Invalid integer for '" + key + "' on line " +
                                 to_string(lineNumber) + ": " + value);
    }

    if (used != value.size() || parsed < numeric_limits<int>::min() ||
        parsed > numeric_limits<int>::max()) {
        throw runtime_error("Invalid integer for '" + key + "' on line " +
                                 to_string(lineNumber) + ": " + value);
    }
    return static_cast<int>(parsed);
}

Config loadConfig(const string& fileName) {
    ifstream input(fileName);
    if (!input) {
        throw runtime_error("Could not open '" + fileName +
                                 "'. Place it in the program's working directory.");
    }

    const vector<string> requiredKeys{"n", "t", "h", "d", "t1", "t2"};
    map<string, int> values;
    string line;
    int lineNumber = 0;

    while (getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3); // Accept UTF-8 files saved with a byte-order mark.
        }

        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const size_t equals = line.find('=');
        if (equals == string::npos || line.find('=', equals + 1) != string::npos) {
            throw runtime_error("Expected key=value syntax on line " +
                                     to_string(lineNumber) + ".");
        }

        const string key = trim(line.substr(0, equals));
        const string value = trim(line.substr(equals + 1));
        if (find(requiredKeys.begin(), requiredKeys.end(), key) == requiredKeys.end()) {
            throw runtime_error("Unknown configuration key '" + key + "' on line " +
                                     to_string(lineNumber) + ".");
        }
        if (values.count(key) != 0) {
            throw runtime_error("Duplicate configuration key '" + key + "' on line " +
                                     to_string(lineNumber) + ".");
        }
        values[key] = parseInteger(value, key, lineNumber);
    }

    for (const string& key : requiredKeys) {
        if (values.count(key) == 0) {
            throw runtime_error("Missing required configuration key '" + key + "'.");
        }
    }

    Config config{values["n"], values["t"], values["h"],
                  values["d"], values["t1"], values["t2"]};
    if (config.n <= 0) {
        throw runtime_error("Invalid configuration: n must be greater than 0.");
    }
    if (config.t < 0 || config.h < 0 || config.d < 0) {
        throw runtime_error("Invalid configuration: t, h, and d must be non-negative.");
    }
    if (config.t1 < 0 || config.t1 > config.t2 || config.t2 > 15) {
        throw runtime_error("Invalid configuration: times must satisfy 0 <= t1 <= t2 <= 15.");
    }
    return config;
}

class Simulation {
public:
    Simulation(const Config& config, int totalParties)
        : config_(config), startTime_(chrono::steady_clock::now()) {
        for (int partyId = 1; partyId <= totalParties; ++partyId) {
            waitingParties_.push(partyId);
        }
        instances_.reserve(config_.n);
        for (int id = 1; id <= config_.n; ++id) {
            instances_.push_back(DungeonInstance{id});
        }
    }

    void run() {
        {
            lock_guard<mutex> lock(mutex_);
            cout << "\n========== DUNGEON SIMULATION ==========\n\n";
            printStatusLocked();
        }

        const int workerCount = min(config_.n, static_cast<int>(waitingParties_.size()));
        vector<thread> workers;
        workers.reserve(workerCount);
        for (int index = 0; index < workerCount; ++index) {
            workers.emplace_back(&Simulation::runInstance, this, index);
        }
        for (thread& worker : workers) {
            worker.join();
        }

        finishTime_ = chrono::steady_clock::now();
    }

    void printFinalSummary() const {
        long long totalServiceTime = 0;
        int totalServed = 0;

        cout << "\n========== FINAL SUMMARY ==========\n\n";
        for (const DungeonInstance& instance : instances_) {
            cout << "Instance " << instance.id << '\n'
                      << "Status          : " << (instance.active ? "active" : "empty") << '\n'
                      << "Parties Served  : " << instance.partiesServed << '\n'
                      << "Total Time      : " << instance.totalTimeServed << " seconds\n\n";
            totalServed += instance.partiesServed;
            totalServiceTime += instance.totalTimeServed;
        }

        const chrono::duration<double> wallTime = finishTime_ - startTime_;
        cout << "Total Parties Served       : " << totalServed << '\n'
                  << "Total Instance Service Time: " << totalServiceTime << " seconds\n"
                  << "Total Simulation Time      : approximately " << fixed
                  << setprecision(3) << wallTime.count() << " seconds\n\n"
                  << "All dungeon parties have been served.\n"
                  << "Simulation completed successfully.\n";
    }

private:
    Config config_;
    queue<int> waitingParties_;
    vector<DungeonInstance> instances_;
    mutable mutex mutex_;
    chrono::steady_clock::time_point startTime_;
    chrono::steady_clock::time_point finishTime_;

    long long elapsedSeconds() const {
        return chrono::duration_cast<chrono::seconds>(
                   chrono::steady_clock::now() - startTime_)
            .count();
    }

    void printStatusLocked() const {
        cout << "--- Current Instance Status ---\n";
        for (const DungeonInstance& instance : instances_) {
            cout << "Instance " << instance.id << ": "
                      << (instance.active ? "active" : "empty");
            if (instance.active) {
                cout << " (Party " << instance.currentPartyId << ')';
            }
            cout << '\n';
        }
        cout << "Waiting parties: " << waitingParties_.size() << "\n\n";
    }

    void runInstance(int instanceIndex) {
        random_device randomDevice;
        seed_seq seed{randomDevice(), randomDevice(),
                           static_cast<unsigned int>(instanceIndex + 1),
                           static_cast<unsigned int>(
                               chrono::steady_clock::now().time_since_epoch().count())};
        mt19937 generator(seed);
        uniform_int_distribution<int> durationDistribution(config_.t1, config_.t2);

        while (true) {
            int partyId = 0;
            int duration = 0;
            {
                lock_guard<mutex> lock(mutex_);
                if (waitingParties_.empty()) {
                    return;
                }

                partyId = waitingParties_.front();
                waitingParties_.pop();
                duration = durationDistribution(generator);

                DungeonInstance& instance = instances_[instanceIndex];
                instance.active = true;
                instance.currentPartyId = partyId;

                cout << '[' << elapsedSeconds() << "s] Party " << partyId
                          << " entered Instance " << instance.id << ".\n"
                          << "     Dungeon duration: " << duration << " seconds\n\n";
                printStatusLocked();
            }

            // Never hold the shared mutex while simulating the dungeon run.
            this_thread::sleep_for(chrono::seconds(duration));

            {
                lock_guard<mutex> lock(mutex_);
                DungeonInstance& instance = instances_[instanceIndex];
                instance.partiesServed += 1;
                instance.totalTimeServed += duration;
                instance.active = false;
                instance.currentPartyId = 0;

                cout << '[' << elapsedSeconds() << "s] Party " << partyId
                          << " completed Instance " << instance.id << ".\n"
                          << "     Instance " << instance.id << " is now available.\n\n";
                printStatusLocked();
            }
        }
    }
};

void printInitialInformation(const Config& config, int totalParties) {
    const int unmatchedTanks = config.t - totalParties;
    const int unmatchedHealers = config.h - totalParties;
    const int unmatchedDps = config.d - totalParties * 3;
    const int totalUnmatched = unmatchedTanks + unmatchedHealers + unmatchedDps;

    cout << "========== MMORPG LFG DUNGEON QUEUE ==========\n\n"
              << "Configuration loaded successfully.\n\n"
              << "Maximum Instances : " << config.n << '\n'
              << "Tanks             : " << config.t << '\n'
              << "Healers           : " << config.h << '\n'
              << "DPS               : " << config.d << '\n'
              << "Minimum Time      : " << config.t1 << " seconds\n"
              << "Maximum Time      : " << config.t2 << " seconds\n\n"
              << "Complete Parties  : " << totalParties << '\n'
              << "Waiting Parties   : " << totalParties << '\n'
              << "Unmatched Players : " << totalUnmatched << '\n'
              << "  Tanks           : " << unmatchedTanks << '\n'
              << "  Healers         : " << unmatchedHealers << '\n'
              << "  DPS             : " << unmatchedDps << '\n';
}

int main() {
    try {
        const Config config = loadConfig("config.txt");
        const int totalParties = min({config.t, config.h, config.d / 3});

        printInitialInformation(config, totalParties);
        Simulation simulation(config, totalParties);
        simulation.run();
        simulation.printFinalSummary();
        return 0;
    } catch (const exception& error) {
        cerr << "Configuration error: " << error.what() << '\n';
        return 1;
    }
}
