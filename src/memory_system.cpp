#include "memory_system.hpp"

AddrParts decodeAddress(const Config &cfg, size_t addr) {
    AddrParts p;
    p.offset   = addr % cfg.lineSize;
    p.lineNum  = addr / cfg.lineSize;
    p.setIndex = p.lineNum % cfg.numSets;
    p.tag      = (uint32_t)(p.lineNum / cfg.numSets);
    return p;
}

void CoreCache::init(const Config &cfg) {
    data.assign(cfg.cacheSize, 0);
    state.assign(cfg.numLines, MesiState::Invalid);
    tag.assign(cfg.numLines, 0);
    plru.assign(cfg.numSets, 0);
    lastUsed.assign(cfg.numLines, -1);
}

int CoreCache::findWay(const Config &cfg, size_t setIndex, uint32_t t) const {
    size_t base = setIndex * cfg.associativity;
    for (size_t w = 0; w < cfg.associativity; ++w) {
        size_t li = base + w;
        if (state[li] != MesiState::Invalid && tag[li] == t) return (int)w;
    }
    return -1;
}

// Tree-based pseudo-LRU (3-bit encoding), used only for the classic 4-way
// case. Kept identical to the scheme in the single-core simulator.
static int plru4SelectVictim(unsigned char bits) {
    int L = bits & 0x1;
    if (L == 0) {
        int A = (bits >> 1) & 0x1;
        return (A == 0) ? 0 : 1;
    } else {
        int B = (bits >> 2) & 0x1;
        return (B == 0) ? 2 : 3;
    }
}

static void plru4Touch(unsigned char &bits, int way) {
    switch (way) {
        case 0: bits = (unsigned char)((bits & ~0x03) | 0x03); break;
        case 1: bits = (unsigned char)((bits & ~0x03) | 0x01); break;
        case 2: bits = (unsigned char)((bits & ~0x05) | 0x04); break;
        case 3: bits = (unsigned char)((bits & ~0x05) | 0x00); break;
        default: break;
    }
}

int CoreCache::selectVictim(const Config &cfg, size_t setIndex) const {
    if (cfg.associativity == 4) {
        return plru4SelectVictim(plru[setIndex]);
    }
    // True-LRU fallback: prefer any never-used (invalid) way first, else the
    // way with the oldest lastUsed timestamp.
    size_t base = setIndex * cfg.associativity;
    int best = 0;
    long long bestTime = lastUsed[base];
    for (size_t w = 0; w < cfg.associativity; ++w) {
        size_t li = base + w;
        if (state[li] == MesiState::Invalid) return (int)w; // empty way wins outright
        if (lastUsed[li] < bestTime) { bestTime = lastUsed[li]; best = (int)w; }
    }
    return best;
}

void CoreCache::touch(const Config &cfg, size_t setIndex, int way, long long now) {
    if (cfg.associativity == 4) {
        plru4Touch(plru[setIndex], way);
    } else {
        lastUsed[setIndex * cfg.associativity + (size_t)way] = now;
    }
}

void Ram::init(const Config &cfg) {
    data.assign(cfg.ramSize, 0);
}

void flushLineToRam(const Config &cfg, Ram &ram, CoreCache &core,
                     size_t lineIdx, size_t setIndex) {
    uint32_t t = core.tag[lineIdx];
    size_t lineNum = (size_t)t * cfg.numSets + setIndex;
    size_t ramBase = lineNum * cfg.lineSize;
    size_t cacheBase = lineIdx * cfg.lineSize;
    for (size_t b = 0; b < cfg.lineSize; ++b) {
        ram.data[ramBase + b] = core.data[cacheBase + b];
    }
}

void loadLineFromRam(const Config &cfg, const Ram &ram, CoreCache &core,
                      size_t lineIdx, size_t lineNum) {
    size_t ramBase = lineNum * cfg.lineSize;
    size_t cacheBase = lineIdx * cfg.lineSize;
    for (size_t b = 0; b < cfg.lineSize; ++b) {
        core.data[cacheBase + b] = ram.data[ramBase + b];
    }
}
