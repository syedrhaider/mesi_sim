#include "stats.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

static double pct(long long num, long long denom) {
    return denom == 0 ? 0.0 : (100.0 * (double)num / (double)denom);
}

void writeStatsReport(const Config &cfg, const CoherentMemorySystem &mem,
                       const std::string &path) {
    std::ofstream out(path);
    if (!out) {
        std::cerr << "Error: cannot open stats file '" << path << "' for writing\n";
        return;
    }

    out << "MESI Multi-Core Cache Simulation - Statistics Report\n";
    out << "======================================================\n\n";
    out << "Configuration:\n";
    out << "  RAM size          : " << cfg.ramSize << " bytes\n";
    out << "  Cache size (/core): " << cfg.cacheSize << " bytes\n";
    out << "  Line size         : " << cfg.lineSize << " bytes\n";
    out << "  Associativity     : " << cfg.associativity << "-way\n";
    out << "  Sets per cache    : " << cfg.numSets << "\n";
    out << "  Cores             : " << cfg.numCores << "\n\n";

    long long totalHits = mem.totalHits();
    long long totalMisses = mem.totalMisses();
    long long totalOps = totalHits + totalMisses;

    out << std::fixed << std::setprecision(2);
    out << "Per-core statistics:\n";
    out << std::left
        << std::setw(6)  << "Core"
        << std::setw(8)  << "Reads"
        << std::setw(8)  << "Writes"
        << std::setw(8)  << "Hits"
        << std::setw(8)  << "Misses"
        << std::setw(10) << "HitRate%"
        << std::setw(10) << "InvSent"
        << std::setw(10) << "InvRecv"
        << std::setw(10) << "WrBacks"
        << std::setw(10) << "C2CXfer"
        << "\n";

    for (size_t i = 0; i < mem.cores().size(); ++i) {
        const CoreCache &c = mem.cores()[i];
        long long ops = c.hits + c.misses;
        out << std::left
            << std::setw(6)  << i
            << std::setw(8)  << c.reads
            << std::setw(8)  << c.writes
            << std::setw(8)  << c.hits
            << std::setw(8)  << c.misses
            << std::setw(10) << pct(c.hits, ops)
            << std::setw(10) << c.invalidationsSent
            << std::setw(10) << c.invalidationsReceived
            << std::setw(10) << c.writebacks
            << std::setw(10) << c.cacheToCacheXfers
            << "\n";
    }

    out << "\nAggregate:\n";
    out << "  Total operations  : " << totalOps << "\n";
    out << "  Total hits        : " << totalHits << "\n";
    out << "  Total misses      : " << totalMisses << "\n";
    out << "  Overall hit rate  : " << pct(totalHits, totalOps) << " %\n";

    double amat = cfg.hitTimeCycles +
                  pct(totalMisses, totalOps) / 100.0 * cfg.missPenaltyCycles;
    out << "\nCost model:\n";
    out << "  Hit time          : " << cfg.hitTimeCycles << " cycles\n";
    out << "  Miss penalty      : " << cfg.missPenaltyCycles << " cycles\n";
    out << "  Estimated AMAT    : " << amat << " cycles\n";

    out.close();
}
