#include "config.h"
#include "simulation.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

using namespace std;

static void waitForEnter(const string& message) {
    cout << '\n' << message;
    string input;
    getline(cin, input);
}

int main() {
    try {
        const Config config = loadConfig("config.txt");
        const int totalParties = min({config.t, config.h, config.d / 3});

        printInitialInformation(config, totalParties);
        waitForEnter("Press Enter to start the simulation...");

        Simulation simulation(config, totalParties);
        simulation.run();
        waitForEnter("Press Enter to view the final summary...");
        simulation.printFinalSummary();

        return 0;
    } catch (const exception& error) {
        cerr << "Configuration error: " << error.what() << '\n';
        return 1;
    }
}
