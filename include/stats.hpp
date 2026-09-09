#pragma once
#include <string>
#include "config.hpp"
#include "coherence.hpp"

// Writes a human-readable per-core + aggregate statistics report, including
// an estimated AMAT (Average Memory Access Time) based on cfg.hitTimeCycles
// and cfg.missPenaltyCycles.
void writeStatsReport(const Config &cfg, const CoherentMemorySystem &mem,
                       const std::string &path);
