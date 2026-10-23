#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

static string trim(const string& text) {
    const auto first = find_if_not(text.begin(), text.end(),
                                   [](unsigned char ch) { return isspace(ch); });
    if (first == text.end()) {
        return {};
    }

    const auto last = find_if_not(text.rbegin(), text.rend(),
                                  [](unsigned char ch) { return isspace(ch); }).base();
    return string(first, last);
}

static int parseInteger(const string& value, const string& key, int lineNumber) {
    if (value.empty()) {
        throw runtime_error("Missing integer value for '" + key + "' on line " +
                            to_string(lineNumber) + ".");
    }

    // Validate the complete value before converting it. This prevents inputs such
    // as "five", "3.5", and "12players" from being partially interpreted as ints.
    const size_t firstDigit = (value.front() == '+' || value.front() == '-') ? 1 : 0;
    const bool hasDigits = firstDigit < value.size();
    const bool containsOnlyDigits =
        hasDigits && all_of(value.begin() + firstDigit, value.end(),
                            [](unsigned char ch) { return isdigit(ch); });
    if (!containsOnlyDigits) {
        throw runtime_error("Expected a whole number for '" + key + "' on line " +
                            to_string(lineNumber) + ", but received '" + value + "'.");
    }

    size_t used = 0;
    long long parsed = 0;
    try {
        parsed = stoll(value, &used, 10);
    } catch (const exception&) {
        throw runtime_error("Integer for '" + key + "' on line " +
                            to_string(lineNumber) + " is outside the supported range: '" +
                            value + "'.");
    }

    if (used != value.size() || parsed < numeric_limits<int>::min() ||
        parsed > numeric_limits<int>::max()) {
        throw runtime_error("Integer for '" + key + "' on line " +
                            to_string(lineNumber) + " is outside the supported range: '" +
                            value + "'.");
    }

    return static_cast<int>(parsed);
}

Config loadConfig(const char* fileName) {
    ifstream input(fileName);
    if (!input) {
        throw runtime_error(string("Could not open '") + fileName +
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
            line.erase(0, 3);
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

    if (config.n <= 0 || config.t < 0 || config.h < 0 || config.d < 0 ||
        config.t1 <= 0 || config.t2 <= 0) {
        throw runtime_error(
            "Invalid configuration: n, t1, and t2 must be greater than 0; "
            "t, h, and d must be 0 or greater.");
    }
    if (config.t1 > config.t2 || config.t2 > 15) {
        throw runtime_error("Invalid configuration: times must satisfy 1 <= t1 <= t2 <= 15.");
    }

    return config;
}

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
