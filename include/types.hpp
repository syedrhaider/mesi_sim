#pragma once
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// MESI coherence states
// ---------------------------------------------------------------------------
enum class MesiState : uint8_t {
    Invalid   = 0,  // I - line not present / not valid in this cache
    Shared    = 1,  // S - clean, possibly present in other caches too
    Exclusive = 2,  // E - clean, guaranteed sole copy (matches RAM)
    Modified  = 3   // M - dirty, sole copy, RAM is stale
};

inline char mesiChar(MesiState s) {
    switch (s) {
        case MesiState::Invalid:   return 'I';
        case MesiState::Shared:    return 'S';
        case MesiState::Exclusive: return 'E';
        case MesiState::Modified:  return 'M';
    }
    return '?';
}

// A single memory operation taken from the interleaved trace file.
struct TraceOp {
    long long   cycle   = 0;    // logical global ordering (bus order)
    int         coreId  = 0;
    char        cmd     = 'R';  // 'R' or 'W'
    uint32_t    addr    = 0;
    uint8_t     value   = 0;
};

// Outcome of processing a single operation, used for logging + a.output style dump.
enum class AccessOutcome : uint8_t { Hit, Miss };

inline char outcomeChar(AccessOutcome o) {
    return o == AccessOutcome::Hit ? 'H' : 'M';
}
