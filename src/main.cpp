#include <iostream>
#include <fstream>
#include "config.hpp"
#include "trace.hpp"
#include "coherence.hpp"
#include "stats.hpp"

int main(int argc, char **argv) {
    Config cfg = loadConfig(argc, argv);

    std::vector<TraceOp> ops = loadTrace(cfg.traceFile);
    CoherentMemorySystem mem(cfg);

    std::string outcomeString;
    outcomeString.reserve(ops.size());

    for (const TraceOp &op : ops) {
        if (op.coreId < 0 || op.coreId >= cfg.numCores) {
            std::cerr << "Warning: skipping op with out-of-range core id "
                      << op.coreId << " at cycle " << op.cycle << "\n";
            continue;
        }
        AccessOutcome outcome = mem.access(op);
        outcomeString.push_back(outcomeChar(outcome));
    }

    std::ofstream fout(cfg.outputFile);
    fout << outcomeString;
    fout.close();

    writeStatsReport(cfg, mem, cfg.statsFile);

    long long hits = mem.totalHits();
    long long misses = mem.totalMisses();
    long long total = hits + misses;
    std::cout << "Processed " << total << " operations across "
              << cfg.numCores << " cores.\n";
    std::cout << "Hits: " << hits << "  Misses: " << misses;
    if (total > 0) {
        std::cout << "  (hit rate " << (100.0 * hits / total) << "%)";
    }
    std::cout << "\n";
    std::cout << "H/M trace  -> " << cfg.outputFile << "\n";
    std::cout << "Stats      -> " << cfg.statsFile << "\n";

    return 0;
}
