#include "link_state.hpp"
#include <iostream>
#include <algorithm>

bool LinkState::update (const Lsp& lsp) {
    auto it = lsp_map.find(lsp.origin);
    if (it != lsp_map.end() && lsp.seq <= it->second.seq) return false;
    lsp_map[lsp.origin] = lsp;
    return true;
}

void LinkState::print() const { //change
    for (const auto& p : lsp_map) {
        const Lsp& lsp = p.second;
        std::cout << "LSP from node " << (char)('A' + lsp.origin) << ", seq: " << lsp.seq << "\n";
        for (const auto& tup : lsp.tuples) {
            std::cout << "  id: " << tup.id
                      << ", ip: " << tup.ip
                      << ", port: " << tup.port
                      << ", cost: " << tup.cost << "\n";
        }
    }
}