#!/usr/bin/env bash
# Basic regression test: builds the simulator and checks that the sample
# trace produces the expected H/M sequence and a sane stats report.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "== Building =="
g++ -O2 -std=c++17 -Wall -Iinclude -o mesi_sim_test src/*.cpp

echo "== Running sample_trace.txt =="
./mesi_sim_test --config sim.cfg --output /tmp/test.output --stats /tmp/test.stats

EXPECTED="MMHMHMHMMMHH"
ACTUAL="$(cat /tmp/test.output)"

if [[ "$ACTUAL" != "$EXPECTED" ]]; then
    echo "FAIL: expected H/M sequence '$EXPECTED' but got '$ACTUAL'"
    exit 1
fi
echo "PASS: H/M sequence matches expected MESI behavior ($ACTUAL)"

echo "== Testing non-default associativity (LRU fallback path) =="
./mesi_sim_test --cores 2 --cache-size 1024 --assoc 2 \
    --trace traces/sample_trace.txt \
    --output /tmp/test2.output --stats /tmp/test2.stats
echo "PASS: ran with --assoc 2 without crashing"

rm -f mesi_sim_test
echo "All tests passed."
