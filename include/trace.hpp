#pragma once
#include <vector>
#include <string>
#include "types.hpp"

// Parses an interleaved multi-core trace file. Each non-blank, non-comment
// line has the format:
//
//   CYCLE CORE_ID CMD ADDR VALUE
//
// e.g.  "0 0 W 0000AA04 AF"
//
// The line order in the file IS the global bus order the simulator uses --
// CYCLE is kept only as an annotation for logging/analysis, not used to
// reorder operations. Lines starting with '#' are treated as comments.
std::vector<TraceOp> loadTrace(const std::string &path);
