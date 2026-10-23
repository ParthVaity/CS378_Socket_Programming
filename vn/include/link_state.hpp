#pragma once

#include <map>
#include <chrono>
#include "common.hpp"

class LinkState {
    std::map<int, Lsp>                                      lspMap;
    std::map<int, std::chrono::steady_clock::time_point>    timeStamp;
public:
    bool                    update (const Lsp& lsp);
    void                    expire (int self, int maxAge);
    void                    print(const char* reason, int self) const;
    void                    printRoutes(int src) const;
};