#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

void recvAll(int fd, uint8_t *buf, int n) {
    int got = 0;
    while (got < n) {
        int r = recv(fd, buf + got, n - got, 0);
        if (r <= 0) { cerr << "connection closed\n"; exit(1); }
        got += r;
    }
}


bool parseConfig (const string& path, int& n, vector<vector<int>>& cost) {
    ifstream f(path);
    if (!f) return false;

    vector<vector<int>> rows;
    string line;

    while(getline(f,line)) {
        int s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos || line[s] == '#') continue;
        
        istringstream ss(line.substr(s));
        vector<int> r;
        int x;
        while(ss>>x) r.push_back(x);
        rows.push_back(r);
    }

    n = rows.size() + 1;
    cost.assign(n, vector<int> (n, -1));
    
    for (int i = 0; i < n; i++) cost[i][i] = 0;
    for (int i = 0; i < rows.size(); i++) {
        for (int k = 0; k < rows[i].size(); k++) {
            int j = i + 1 + k;
            if (j >= n) return false;
            cost[i][j] = cost[j][i] = rows[i][k];
        }
    }
    
    return true;
}


int main(int argc, char **argv) {
    
    if (argc != 2) { cerr << "usage: ./on <config-file>\n"; return 1; }
    string path = argv[1];
    
    vector<vector<int>> cost;
    int n;

    if(!parseConfig(path, n, cost)) {cerr << "Wrong config file format\n"; }


    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(5000);
    if (bind(srv, (sockaddr *)&a, sizeof a) < 0) { perror("bind"); return 1; }
    listen(srv, 6);
    cout << "waiting for " << n << " nodes\n";

    vector<int> conn(n);
    vector<vector<uint8_t>> addr(n, vector<uint8_t>(6));   
    for (int i = 0; i < n; i++) {
        conn[i] = accept(srv, nullptr, nullptr);
        uint8_t msg[7];                                    
        recvAll(conn[i], msg, 7);
        memcpy(addr[i].data(), msg + 1, 6);
        cout << "node " << char('A' + i) << " connected\n";
    }

    for (int i = 0; i < n; i++) close(conn[i]);
    close(srv);
    return 0;
}