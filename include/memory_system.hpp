#pragma once
#include <vector>
#include <cstdint>
#include "types.hpp"
#include "config.hpp"

// Decomposition of an address into cache coordinates.
struct AddrParts {
    size_t   lineNum;
    size_t   setIndex;
    uint32_t tag;
    size_t   offset;
};

AddrParts decodeAddress(const Config &cfg, size_t addr);

// ---------------------------------------------------------------------------
// One core's private L1 cache. Data is a flat byte array (as in real
// hardware); per-line bookkeeping (state/tag) are parallel arrays.
// Pseudo-LRU (3-bit tree encoding) is used for victim selection, exactly as
// in the single-core simulator, but now kept per core.
// ---------------------------------------------------------------------------
struct CoreCache {
    std::vector<unsigned char> data;   // cacheSize bytes
    std::vector<MesiState>     state;  // numLines
    std::vector<uint32_t>      tag;    // numLines
    std::vector<unsigned char> plru;   // numSets, 3 bits used per set (assoc == 4 only)
    std::vector<long long>     lastUsed; // numLines, used for true-LRU fallback (assoc != 4)

    // Per-core statistics.
    long long reads            = 0;
    long long writes           = 0;
    long long hits             = 0;
    long long misses           = 0;
    long long invalidationsSent     = 0; // times this core forced another core to invalidate
    long long invalidationsReceived = 0; // times this core's line was invalidated by another
    long long writebacks        = 0;     // times this core flushed dirty data to RAM
    long long cacheToCacheXfers  = 0;    // times this core supplied data to another core

    void init(const Config &cfg);

    // Find a way in the given set whose tag matches and whose state != Invalid.
    // Returns -1 if not found.
    int findWay(const Config &cfg, size_t setIndex, uint32_t tag) const;

    // Select a victim way in the given set. Uses tree-based pseudo-LRU when
    // associativity == 4 (matching the classic 3-bit scheme), and true LRU
    // (via lastUsed timestamps) for any other associativity.
    int  selectVictim(const Config &cfg, size_t setIndex) const;
    void touch(const Config &cfg, size_t setIndex, int way, long long now);

    size_t lineIndex(size_t setIndex, int way, const Config &cfg) const {
        return setIndex * cfg.associativity + (size_t)way;
    }
};

// Global RAM, modeled as a flat byte array shared by all cores.
struct Ram {
    std::vector<unsigned char> data;
    void init(const Config &cfg);
};

// Copy one full cache line from a core's cache out to RAM (write-back).
void flushLineToRam(const Config &cfg, Ram &ram, CoreCache &core,
                     size_t lineIdx, size_t setIndex);

// Copy one full cache line from RAM into a core's cache slot.
void loadLineFromRam(const Config &cfg, const Ram &ram, CoreCache &core,
                      size_t lineIdx, size_t lineNum);
