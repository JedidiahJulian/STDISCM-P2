#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"

#include <chrono>
#include <mutex>
#include <queue>
#include <vector>

using namespace std;

class Simulation {
public:
    Simulation(const Config& config, int totalParties);

    void run();
    void printFinalSummary() const;

private:
    struct DungeonInstance {
        int id{};
        bool active{false};
        int currentPartyId{0};
        int partiesServed{0};
        int totalTimeServed{0};
    };

    Config config_;
    queue<int> waitingParties_;
    vector<DungeonInstance> instances_;
    mutable mutex mutex_;
    chrono::steady_clock::time_point startTime_;
    chrono::steady_clock::time_point finishTime_;

    long long elapsedSeconds() const;
    void printStatusLocked() const;
    void runInstance(int instanceIndex);
};

#endif
