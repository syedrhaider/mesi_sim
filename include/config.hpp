#pragma once
#include <cstddef>
#include <string>

// All tunable simulation parameters live here. Values are seeded from a
// key=value config file and can be overridden individually on the command
// line, e.g.:
//
//   ./mesi_sim --config sim.cfg --cores 8 --cache-size 65536
//
struct Config {
    size_t ramSize          = 1048576;   // bytes
    size_t cacheSize        = 32768;     // bytes, per core
    size_t lineSize         = 64;        // bytes
    size_t associativity    = 4;         // ways per set
    int    numCores         = 4;

    std::string traceFile   = "traces/sample_trace.txt";
    std::string outputFile  = "trace.output";  // H/M string, one char per op
    std::string statsFile   = "stats.txt";     // detailed per-core statistics

    // Cost model used to report an estimated AMAT (Average Memory Access Time).
    double hitTimeCycles    = 1.0;
    double missPenaltyCycles = 50.0;

    bool verbose = false;   // print a per-operation coherence trace to stdout

    // Derived (computed after load, not read directly from file/CLI).
    size_t numLines = 0;
    size_t numSets  = 0;

    void finalize();  // computes numLines / numSets, validates values
};

// Load defaults, then apply a config file (if it exists), then apply CLI args.
// Prints usage and exits on --help or on a fatal parsing error.
Config loadConfig(int argc, char** argv);
