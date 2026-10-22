#include "link_state.hpp"
#include <iostream>
#include <algorithm>
using namespace std;

bool LinkState::update (const Lsp& lsp) {
    auto it = lsp_map.find(lsp.origin);
    if (it != lsp_map.end() && lsp.seq <= it->second.seq) return false;
    lsp_map[lsp.origin] = lsp;
    return true;
}

void LinkState::print() const {
    int N = 0;
    for (auto& l:lsp_map) {
        N = max(N, l.first+1);
        for (auto& t:l.second.tuples) {
            N = max(N, (int)(t.id-'A'+1));
        }
    }
    vector<vector<int>> m(N, vector<int>(N, -1));
    for (auto& l:lsp_map) {
        for(auto& t:l.second.tuples) {
            if(t.cost > 0) m[l.first][t.id-'A'] = t.cost;
        }
    }
    cout << "Current Link State:\n";
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            if (i==j) cout<<"0\t";
            else if (m[i][j] == -1) cout << "X\t";
            else cout << m[i][j] << "\t";
        }
        cout << "\n";
    }
}