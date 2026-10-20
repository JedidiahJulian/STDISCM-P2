#ifndef CONFIG_H
#define CONFIG_H

struct Config {
    int n{};
    int t{};
    int h{};
    int d{};
    int t1{};
    int t2{};
};

Config loadConfig(const char* fileName);
void printInitialInformation(const Config& config, int totalParties);

#endif
