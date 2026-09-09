#include "trace.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

std::vector<TraceOp> loadTrace(const std::string &path) {
    std::vector<TraceOp> ops;
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "Error: cannot open trace file '" << path << "'\n";
        std::exit(1);
    }
    std::string line;
    size_t lineNo = 0;
    while (std::getline(fin, line)) {
        ++lineNo;
        // strip comments / blank lines
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream iss(line);
        TraceOp op;
        std::string cmdStr, addrStr, valStr;
        if (!(iss >> op.cycle >> op.coreId >> cmdStr >> addrStr >> valStr)) {
            continue; // blank or malformed line, skip
        }
        op.cmd = cmdStr.empty() ? 'R' : cmdStr[0];
        op.addr = (uint32_t)std::strtoul(addrStr.c_str(), nullptr, 16);
        op.value = (uint8_t)std::strtoul(valStr.c_str(), nullptr, 16);
        ops.push_back(op);
    }
    return ops;
}
