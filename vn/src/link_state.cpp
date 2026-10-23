#include "link_state.hpp"
#include <iostream>
#include <algorithm>
#include <queue>
#include <climits>
using namespace std;

bool LinkState::update (const Lsp& lsp) {
    auto it = lspMap.find(lsp.origin);
    if (it != lspMap.end() && lsp.seq <= it->second.seq) return false;
    lspMap[lsp.origin] = lsp;
    timeStamp[lsp.origin] = time(nullptr);
    return true;
}

void LinkState::expire (int self, int maxAge) {
    time_t now = time(nullptr);
    for (auto it = lspMap.begin(); it != lspMap.end();) {
        if (it->first != self && now - timeStamp[it->first] > maxAge) {
            timeStamp.erase(it->first);
            it = lspMap.erase(it);
        }
        else it++;
    }
}

static void buildComponent(const map<int,Lsp>& lspMap, int self, vector<vector<int>>& m, vector<int>& ids) {
    m.assign(MAXN, vector<int>(MAXN, -1));
    for(auto& l:lspMap) {
        for(auto& t:l.second.tuples) {
            if(t.cost > 0) m[l.first][t.id - 'A'] = t.cost;
        }
    }

    auto linked = [&](int i, int j) {return m[i][j] > 0 && m[j][i] > 0;};

    vector<bool> seen(MAXN, false);
    queue<int> q;
    if(self >= 0 && self < MAXN) {
        seen[self] = true;
        q.push(self);
    }
    while(!q.empty()) {
        int u=q.front();
        q.pop();
        for(int v = 0; v < MAXN; v++){
            if (!seen[v] && linked(u,v)) {
                seen[v] = true;
                q.push(v);
            }
        }
    }
    ids.clear();
    for(int i = 0; i < MAXN; i++) if(seen[i]) ids.push_back(i);
}

void LinkState::print(const char* reason, int self) const {
    vector<vector<int>> m;
    vector<int> ids;
    buildComponent(lspMap, self, m, ids);

    auto linked = [&](int i, int j) {return m[i][j] > 0 && m[j][i] > 0;};
    int N = ids.size();

    cout << "Current Visible Graph State: (" << reason << ")\n";
    cout<<"\t";
    for(int i = 0; i < N; i++) cout << char('A'+ids[i]) << ((i + 1 < N) ? "\t" : "\n");
    for(int i = 0; i < N; i++) {
        cout << char('A' + ids[i]) <<"\t";
        for (int j = 0; j < N ;j++) {
            int a = ids[i];
            int b = ids[j];
            int c = (i==j) ? 0 : (linked(a,b) ? max(m[a][b], m[b][a]) : -1);
            cout << c << ((j + 1 < N) ? "\t" : "\n" );
        }
    }
}

void LinkState::printRoutes(int src) const {
    if (src < 0 || src >= MAXN) return;
    vector<vector<int>> m;
    vector<int> ids;
    buildComponent(lspMap, src, m, ids);

    if(ids.size() <= 1) {
        cout<<"Shortest Routes from VN "<< char('A' + src)<< ": no reachable neighbours\n";
        return;
    }
    auto linked = [&](int i, int j) {return m[i][j] > 0 && m[j][i] > 0;};


    vector<long> dist(MAXN, LONG_MAX);
    vector<int> prev(MAXN, -1);
    priority_queue<pair<long,int>,vector<pair<long,int>>,greater<pair<long,int>>> pq;
    dist[src] = 0;
    pq.push({0,src});
    while(!pq.empty()) {
        auto [d,u] = pq.top();
        pq.pop();
        if(d > dist[u]) continue;
        for(int v = 0; v < MAXN; v++) {
            if(!linked(u,v)) continue;
            int c = max(m[u][v], m[v][u]);
            if(dist[v] > d + c) {
                dist[v] = d + c;
                prev[v] = u;
                pq.push({dist[v],v});
            }
        }
    }

    cout<<"Shortest Routes from VN "<<char('A'+src)<<"\n";
    for(int id:ids) {
        if(id==src) continue;
        int hop = id;
        while(prev[hop] != src) hop = prev[hop];
        cout << char('A'+id) << ": cost " << dist[id] <<", next hop " << char('A'+hop) << "\n";    
    }
    cout.flush();
}