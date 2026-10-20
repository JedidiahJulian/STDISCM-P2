#include "config.h"
#include "simulation.h"

#include <algorithm>
#include <exception>
#include <iostream>

using namespace std;

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
