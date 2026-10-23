#pragma once

#include <map>
#include <ctime>
#include "common.hpp"

class LinkState {
    std::map<int, Lsp> lspMap;
    std::map<int, time_t> timeStamp;
public:
    bool update (const Lsp& lsp);
    void expire (int self, int maxAge);
    void print(const char* reason, int self) const;
    void printRoutes(int src) const;
};