#include "link_state.hpp"
#include <iostream>
#include <algorithm>
#include <queue>
#include <climits>
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

void LinkState::printRoutes(int src) const {
    int N = 0;
    for (auto& l:lsp_map) {
        N = max(N, l.first+1);
        for (auto& t:l.second.tuples) {
            N = max(N, (int)(t.id-'A'+1));
        }
    }
    if (src < 0 || src >= N) return;
    vector<vector<int>> m(N, vector<int>(N, -1));
    for (auto& l:lsp_map) {
        for(auto& t:l.second.tuples) {
            if(t.cost > 0) m[l.first][t.id-'A'] = t.cost;
        }
    }

    vector<long> dist(N, LONG_MAX);
    vector<int> prev(N, -1);
    priority_queue<pair<int,int>,vector<pair<int,int>>,greater<pair<int,int>>> pq;
    dist[src] = 0;
    pq.push({0,src});
    while(!pq.empty()) {
        auto [d,u] = pq.top();
        pq.pop();
        if(d > dist[u]) continue;
        for(int v = 0; v < N; v++) {
            if(m[u][v] < 0) continue;
            if(dist[v] > d + m[u][v]) {
                dist[v] = d + m[u][v];
                prev[v] = u;
                pq.push({dist[v],v});
            }
        }
    }

    cout<<"Shortest Routes from VN "<<char('A'+src)<<"\n";
    for(int d = 0; d < N; d++) {
        if(d==src) continue;
        if(dist[d]==LONG_MAX) {
            cout<<char('A'+d)<<" is unreachable\n";
            continue;
        }
        int hop = d;
        while(prev[hop] != src) hop = prev[hop];
        cout << char('A'+d) << ": cost " << dist[d] <<", nexxt hop " << char('A'+hop) << "\n";    
    }
    cout.flush();
}