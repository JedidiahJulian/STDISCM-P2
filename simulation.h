#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"

#include <chrono>
#include <mutex>
#include <queue>
#include <semaphore>
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
        vector<int> servedPartyIds;
    };

    Config config_;
    queue<int> waitingParties_;
    vector<DungeonInstance> instances_;
    mutable mutex mutex_;
    counting_semaphore<> availableInstances_;
    chrono::steady_clock::time_point startTime_;

    long long elapsedSeconds() const;
    void printStatusLocked() const;
    void runParty(int instanceIndex, int partyId, int duration);
};

#endif
