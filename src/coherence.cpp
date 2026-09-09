#include "coherence.hpp"
#include <iostream>
#include <iomanip>

CoherentMemorySystem::CoherentMemorySystem(const Config &cfg) : cfg_(cfg) {
    cores_.resize((size_t)cfg.numCores);
    for (auto &c : cores_) c.init(cfg);
    ram_.init(cfg);
}

long long CoherentMemorySystem::totalHits() const {
    long long s = 0; for (auto &c : cores_) s += c.hits; return s;
}
long long CoherentMemorySystem::totalMisses() const {
    long long s = 0; for (auto &c : cores_) s += c.misses; return s;
}

int CoherentMemorySystem::evictAndPrepare(int coreId, size_t setIndex) {
    CoreCache &me = cores_[(size_t)coreId];
    int victimWay = me.selectVictim(cfg_, setIndex);
    size_t li = me.lineIndex(setIndex, victimWay, cfg_);
    if (me.state[li] != MesiState::Invalid) {
        if (me.state[li] == MesiState::Modified) {
            flushLineToRam(cfg_, ram_, me, li, setIndex);
            me.writebacks++;
        }
        me.state[li] = MesiState::Invalid;
    }
    return victimWay;
}

bool CoherentMemorySystem::snoopInvalidateOthers(int requesterId, size_t setIndex,
                                                  uint32_t tag, bool &otherWasModified) {
    bool any = false;
    otherWasModified = false;
    for (size_t c = 0; c < cores_.size(); ++c) {
        if ((int)c == requesterId) continue;
        CoreCache &other = cores_[c];
        int w = other.findWay(cfg_, setIndex, tag);
        if (w == -1) continue;
        size_t li = other.lineIndex(setIndex, w, cfg_);
        any = true;
        if (other.state[li] == MesiState::Modified) {
            flushLineToRam(cfg_, ram_, other, li, setIndex);
            other.writebacks++;
            otherWasModified = true;
        }
        other.state[li] = MesiState::Invalid;
        other.invalidationsReceived++;
        cores_[(size_t)requesterId].invalidationsSent++;
    }
    return any;
}

bool CoherentMemorySystem::snoopForRead(int requesterId, size_t setIndex, uint32_t tag) {
    bool anyOther = false;
    for (size_t c = 0; c < cores_.size(); ++c) {
        if ((int)c == requesterId) continue;
        CoreCache &other = cores_[c];
        int w = other.findWay(cfg_, setIndex, tag);
        if (w == -1) continue;
        anyOther = true;
        size_t li = other.lineIndex(setIndex, w, cfg_);
        if (other.state[li] == MesiState::Modified) {
            // Owner supplies fresh data: flush to RAM, then downgrade to Shared.
            flushLineToRam(cfg_, ram_, other, li, setIndex);
            other.writebacks++;
            other.state[li] = MesiState::Shared;
            other.cacheToCacheXfers++;
        } else if (other.state[li] == MesiState::Exclusive) {
            other.state[li] = MesiState::Shared;
        }
        // If already Shared, no change needed.
    }
    return anyOther;
}

static void logLine(const TraceOp &op, AccessOutcome outcome, MesiState finalState,
                     const std::string &note) {
    std::cout << "[cycle " << std::setw(5) << op.cycle << "] "
              << "core" << op.coreId << " "
              << op.cmd << " 0x" << std::hex << std::setw(8) << std::setfill('0')
              << op.addr << std::dec << std::setfill(' ')
              << " -> " << (outcome == AccessOutcome::Hit ? "HIT " : "MISS")
              << " new_state=" << mesiChar(finalState)
              << "  (" << note << ")\n";
}

AccessOutcome CoherentMemorySystem::access(const TraceOp &op) {
    ++clock_;
    AddrParts p = decodeAddress(cfg_, (size_t)op.addr);
    CoreCache &me = cores_[(size_t)op.coreId];
    int way = me.findWay(cfg_, p.setIndex, p.tag);
    bool isWrite = (op.cmd == 'W' || op.cmd == 'w');

    if (way != -1) {
        // ------------------------------- HIT -------------------------------
        size_t li = me.lineIndex(p.setIndex, way, cfg_);
        me.hits++;
        std::string note = "hit";

        if (isWrite) {
            me.writes++;
            MesiState st = me.state[li];
            if (st == MesiState::Shared) {
                bool otherWasModified = false; // unused here; already Shared implies clean
                snoopInvalidateOthers(op.coreId, p.setIndex, p.tag, otherWasModified);
                me.state[li] = MesiState::Modified;
                note = "write hit, S->M upgrade (invalidated other sharers)";
            } else if (st == MesiState::Exclusive) {
                me.state[li] = MesiState::Modified;
                note = "write hit, E->M";
            } else {
                note = "write hit, already M";
            }
            me.data[li * cfg_.lineSize + p.offset] = op.value;
        } else {
            me.reads++;
            note = "read hit";
        }
        me.touch(cfg_, p.setIndex, way, clock_);
        if (cfg_.verbose) logLine(op, AccessOutcome::Hit, me.state[li], note);
        return AccessOutcome::Hit;
    }

    // ------------------------------- MISS -------------------------------
    me.misses++;
    MesiState finalState;
    std::string note;

    if (isWrite) {
        me.writes++;
        bool otherWasModified = false;
        bool anyOther = snoopInvalidateOthers(op.coreId, p.setIndex, p.tag, otherWasModified);
        int victimWay = evictAndPrepare(op.coreId, p.setIndex);
        size_t li = me.lineIndex(p.setIndex, victimWay, cfg_);
        loadLineFromRam(cfg_, ram_, me, li, p.lineNum);
        me.data[li * cfg_.lineSize + p.offset] = op.value;
        me.tag[li] = p.tag;
        me.state[li] = MesiState::Modified;
        me.touch(cfg_, p.setIndex, victimWay, clock_);
        finalState = MesiState::Modified;
        note = anyOther ? "write miss (RFO), invalidated other copies" : "write miss (RFO), no other copies";
    } else {
        me.reads++;
        bool anyOther = snoopForRead(op.coreId, p.setIndex, p.tag);
        int victimWay = evictAndPrepare(op.coreId, p.setIndex);
        size_t li = me.lineIndex(p.setIndex, victimWay, cfg_);
        loadLineFromRam(cfg_, ram_, me, li, p.lineNum);
        me.tag[li] = p.tag;
        me.state[li] = anyOther ? MesiState::Shared : MesiState::Exclusive;
        me.touch(cfg_, p.setIndex, victimWay, clock_);
        finalState = me.state[li];
        note = anyOther ? "read miss, shared with other core(s)" : "read miss, sole copy (Exclusive)";
    }

    if (cfg_.verbose) logLine(op, AccessOutcome::Miss, finalState, note);
    return AccessOutcome::Miss;
}
