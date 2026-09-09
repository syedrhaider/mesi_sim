#pragma once
#include <vector>
#include "config.hpp"
#include "types.hpp"
#include "memory_system.hpp"

// Owns the whole memory system (RAM + one CoreCache per core) and implements
// the MESI snooping protocol. Because the input trace already gives a single
// global ordering of operations across all cores, each request can be
// processed as an atomic bus transaction -- exactly what a simple in-order
// snooping bus provides. This keeps the model simple while remaining
// structurally faithful to real MESI transition rules.
class CoherentMemorySystem {
public:
    explicit CoherentMemorySystem(const Config &cfg);

    // Process one trace operation. Returns Hit or Miss for the requesting
    // core. If cfg.verbose is set, prints a one-line description of the
    // resulting bus activity (state transitions, invalidations, flushes).
    AccessOutcome access(const TraceOp &op);

    // Global (summed) statistics, useful for a top-level report.
    long long totalHits() const;
    long long totalMisses() const;

    const std::vector<CoreCache> &cores() const { return cores_; }
    const Ram &ram() const { return ram_; }

private:
    const Config &cfg_;
    std::vector<CoreCache> cores_;
    Ram ram_;
    long long clock_ = 0; // monotonically increasing counter for LRU fallback

    // Evict a victim line in `coreId`'s cache at (setIndex) to make room for
    // a new tag, writing it back to RAM first if it is Modified. Returns the
    // way index used.
    int evictAndPrepare(int coreId, size_t setIndex);

    // Invalidate any *other* core's copy of the given (setIndex, tag) line.
    // If that copy is Modified, it is flushed to RAM first. Returns true if
    // any other core actually held the line (was a coherence-relevant miss).
    bool snoopInvalidateOthers(int requesterId, size_t setIndex, uint32_t tag,
                                bool &otherWasModified);

    // For a read miss: look at other cores' copies to decide the resulting
    // state for the requester (Shared vs Exclusive), performing any needed
    // flush/downgrade on the current owner. Returns true if at least one
    // other valid copy existed.
    bool snoopForRead(int requesterId, size_t setIndex, uint32_t tag);
};
