#include "simulation.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

using namespace std;

Simulation::Simulation(const Config& config, int totalParties)
    : config_(config), availableInstances_(config.n),
      startTime_(chrono::steady_clock::now()) {
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

    vector<thread> workers;
    workers.reserve(waitingParties_.size());

    srand(static_cast<unsigned int>(time(nullptr)));

    while (!waitingParties_.empty()) {
        // Each permit represents one empty dungeon instance. This blocks the
        // dispatcher without busy-waiting when all instances are occupied.
        availableInstances_.acquire();

        int partyId = 0;
        int instanceIndex = 0;
        const int duration =
            config_.t1 + rand() % (config_.t2 - config_.t1 + 1);

        {
            lock_guard<mutex> lock(mutex_);
            partyId = waitingParties_.front();
            waitingParties_.pop();

            const auto available = find_if(
                instances_.begin(), instances_.end(),
                [](const DungeonInstance& instance) { return !instance.active; });
            instanceIndex = static_cast<int>(available - instances_.begin());

            DungeonInstance& instance = instances_[instanceIndex];
            instance.active = true;
            instance.currentPartyId = partyId;

            cout << '[' << elapsedSeconds() << "s] Party " << partyId
                 << " entered Instance " << instance.id << ".\n"
                 << "     Dungeon duration: " << duration << " seconds\n\n";
            printStatusLocked();
        }

        workers.emplace_back(
            &Simulation::runParty, this, instanceIndex, partyId, duration);
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

void Simulation::runParty(int instanceIndex, int partyId, int duration) {
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

    availableInstances_.release();
}
