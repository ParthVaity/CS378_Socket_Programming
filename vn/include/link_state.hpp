#pragma once

#include <map>
#include "common.hpp"

class LinkState {
    std::map<int, Lsp> lsp_map;
public:
    bool update (const Lsp& lsp);
    void print() const;
    void printRoutes(int src) const;
};