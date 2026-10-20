#include "simulation.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

Simulation::Simulation(const Config& config, int totalParties)
    : config_(config), startTime_(chrono::steady_clock::now()) {
    for (int partyId = 1; partyId <= totalParties; ++partyId) {
        waitingParties_.push(partyId);
    }

    instances_.reserve(config_.n);
    for (int id = 1; id <= config_.n; ++id) {
        instances_.push_back(DungeonInstance{id});
    }
}

void Simulation::run() {
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

void Simulation::printFinalSummary() const {
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

long long Simulation::elapsedSeconds() const {
    return chrono::duration_cast<chrono::seconds>(
               chrono::steady_clock::now() - startTime_)
        .count();
}

void Simulation::printStatusLocked() const {
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

void Simulation::runInstance(int instanceIndex) {
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

        // Other workers can access shared state while this dungeon is running.
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
