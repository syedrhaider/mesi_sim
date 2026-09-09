#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void Config::finalize() {
    if (lineSize == 0 || cacheSize == 0 || ramSize == 0 || associativity == 0) {
        std::cerr << "Error: ram/cache/line size and associativity must be non-zero\n";
        std::exit(1);
    }
    numLines = cacheSize / lineSize;
    if (numLines < associativity || numLines % associativity != 0) {
        std::cerr << "Error: cache_size/line_size must be a multiple of associativity\n";
        std::exit(1);
    }
    numSets = numLines / associativity;
    if (numCores < 1) {
        std::cerr << "Error: num_cores must be >= 1\n";
        std::exit(1);
    }
}

static void applyKeyValue(Config &cfg, const std::string &key, const std::string &val) {
    if (key == "ram_size")            cfg.ramSize = std::stoul(val);
    else if (key == "cache_size")     cfg.cacheSize = std::stoul(val);
    else if (key == "line_size")      cfg.lineSize = std::stoul(val);
    else if (key == "associativity")  cfg.associativity = std::stoul(val);
    else if (key == "num_cores")      cfg.numCores = std::stoi(val);
    else if (key == "trace_file")     cfg.traceFile = val;
    else if (key == "output_file")    cfg.outputFile = val;
    else if (key == "stats_file")     cfg.statsFile = val;
    else if (key == "hit_time_cycles")     cfg.hitTimeCycles = std::stod(val);
    else if (key == "miss_penalty_cycles") cfg.missPenaltyCycles = std::stod(val);
    else if (key == "verbose")        cfg.verbose = (val == "1" || val == "true" || val == "yes");
    else std::cerr << "Warning: unknown config key '" << key << "' ignored\n";
}

static void loadConfigFile(Config &cfg, const std::string &path) {
    std::ifstream fin(path);
    if (!fin) return; // config file is optional; silently skip if absent
    std::string line;
    while (std::getline(fin, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(trimmed.substr(0, eq));
        std::string val = trim(trimmed.substr(eq + 1));
        if (!key.empty() && !val.empty()) applyKeyValue(cfg, key, val);
    }
}

static void printUsage(const char *prog) {
    std::cout <<
        "Usage: " << prog << " [--config PATH] [options]\n\n"
        "Options (override config file / defaults):\n"
        "  --config PATH            key=value config file (default: sim.cfg if present)\n"
        "  --ram-size N             RAM size in bytes\n"
        "  --cache-size N           per-core cache size in bytes\n"
        "  --line-size N            cache line size in bytes\n"
        "  --assoc N                set associativity (ways per set)\n"
        "  --cores N                number of cores\n"
        "  --trace PATH             interleaved trace file\n"
        "  --output PATH            H/M output file\n"
        "  --stats PATH             statistics output file\n"
        "  --hit-time N             hit time in cycles (for AMAT estimate)\n"
        "  --miss-penalty N         miss penalty in cycles (for AMAT estimate)\n"
        "  --verbose                print per-operation coherence trace to stdout\n"
        "  --help                   show this message\n";
}

Config loadConfig(int argc, char **argv) {
    Config cfg;

    // 1) Load a default config file if present (without requiring --config).
    loadConfigFile(cfg, "sim.cfg");

    // 2) Explicit --config PATH takes precedence and re-applies over defaults.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            loadConfigFile(cfg, argv[i + 1]);
        }
    }

    // 3) Individual CLI flags override whatever the config file set.
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char *flagName) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << flagName << " requires a value\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if (a == "--help" || a == "-h") { printUsage(argv[0]); std::exit(0); }
        else if (a == "--config") { ++i; /* already handled above */ }
        else if (a == "--ram-size")      cfg.ramSize = std::stoul(next("--ram-size"));
        else if (a == "--cache-size")    cfg.cacheSize = std::stoul(next("--cache-size"));
        else if (a == "--line-size")     cfg.lineSize = std::stoul(next("--line-size"));
        else if (a == "--assoc")         cfg.associativity = std::stoul(next("--assoc"));
        else if (a == "--cores")         cfg.numCores = std::stoi(next("--cores"));
        else if (a == "--trace")         cfg.traceFile = next("--trace");
        else if (a == "--output")        cfg.outputFile = next("--output");
        else if (a == "--stats")         cfg.statsFile = next("--stats");
        else if (a == "--hit-time")      cfg.hitTimeCycles = std::stod(next("--hit-time"));
        else if (a == "--miss-penalty")  cfg.missPenaltyCycles = std::stod(next("--miss-penalty"));
        else if (a == "--verbose")       cfg.verbose = true;
        else {
            std::cerr << "Warning: unrecognized argument '" << a << "'\n";
        }
    }

    cfg.finalize();
    return cfg;
}
