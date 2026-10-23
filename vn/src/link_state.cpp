#include "link_state.hpp"
#include <iostream>
#include <algorithm>
#include <queue>
#include <climits>
#include <set>
using namespace std;

bool LinkState::update (const Lsp& lsp) {
    auto it = lsp_map.find(lsp.origin);
    if (it != lsp_map.end() && lsp.seq <= it->second.seq) return false;
    lsp_map[lsp.origin] = lsp;
    return true;
}

void LinkState::print(const char* reason) const {
    set<int> seen;
    for (auto& l:lsp_map) {
        seen.insert(l.first);
        for (auto& t:l.second.tuples) {
            seen.insert(t.id-'A');
        }
    }
    vector<int> ids(seen.begin(),seen.end());
    int N = ids.size();
    vector<vector<int>> m(N, vector<int>(N, -1));
    for(int i = 0; i < N; i++) m[i][i] = 0;
    for (auto& l:lsp_map) {
        for(auto& t:l.second.tuples) {
            int i = lower_bound(ids.begin(),ids.end(), l.first) - ids.begin();
            int j = lower_bound(ids.begin(),ids.end(), t.id - 'A') - ids.begin();
            if(i != j && t.cost > 0) m[i][j] = t.cost;
        }
    }
    cout << "Current Graph State: (" << reason << ")\n";
    cout<<"\t";
    for(int i = 0; i < N; i++) cout << char('A'+ids[i]) << ((i + 1 < N) ? "\t" : "\n");
    for(int i = 0; i < N; i++) {
        cout << char('A' + ids[i]) <<"\t";
        for (int j = 0; j < N ;j++) {
            cout << m[i][j] << ((j + 1 < N) ? "\t" : "\n");
        }
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