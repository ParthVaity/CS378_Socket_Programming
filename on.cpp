#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
using namespace std;

const int ON_PORT   = 5000;
const int MAXN      = 26;

struct VN{
    int fd;
    uint8_t addr[6];
};


// Socket Functions

int createSocket() {
    int fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    return fd;
}

void bindSocket(int fd, int port) {
    sockaddr_in a{};
    a.sin_family =  AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(port);
    if(bind(fd, (sockaddr*)&a, sizeof a) < 0) { perror("bind"); exit(1); }
}

void listenSocket(int fd, int logs) {
    if (listen(fd, logs) < 0) { perror("listen"); exit(1); }
}

int acceptClient(int fd) {
    return accept(fd, nullptr, nullptr);
}

bool recvAll(int fd, uint8_t *buf, int n) {
    int got = 0;
    while (got < n) {
        int r = recv(fd, buf + got, n - got, 0);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

bool sendAll(int fd, const uint8_t *buf, int n) {
    int sent = 0;
    while (sent < n) {
        int s = send(fd, buf + sent, n - sent, 0);
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}

void closeSocket(int fd) {
    close(fd);
}


// Config File

string readFile(const string &path) {
    ifstream f(path);
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool parseConfig (const string& text, int& n, vector<vector<int>>& cost) {
    istringstream in(text);
    vector<vector<int>> rows;
    string line;
    while(getline(in,line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos || line[s] == '#') continue;
        istringstream ss(line.substr(s));
        vector<int> r;
        int x;
        while(ss>>x) r.push_back(x);
        rows.push_back(r);
    }
    n = rows.size();
    if (n == 0 || n + 1 > MAXN) return false;
    for (int i = 0; i < rows.size(); i++) {
        if(rows[i].size() != n - i) return false;
        for (int c : rows[i]) if (c == 0 || c < -1) return false;
    }

    n++;
    cost.assign(n, vector<int> (n, -1));
    for (int i = 0; i < n; i++) cost[i][i] = 0;
    for (int i = 0; i < rows.size(); i++) {
        for (int k = 0; k < rows[i].size(); k++) {
            int j = i + 1 + k;
            cost[i][j] = cost[j][i] = rows[i][k];
        }
    }
    return true;
}

void checkConfig (const string &path, string& lastConfig, int &n, vector<vector<int>> &cost, const vector<VN> &vns, bool ready) {
    string curr = readFile(path);
    if (curr.empty() || curr == lastConfig) return;
    lastConfig = curr;
    cout << "Config updated\n";
    int n2;
    vector<vector<int>> cost2;
    if(!parseConfig(curr, n2, cost2) || n2 != n) {
        cerr << "Bad config\n";
        return;
    }
    cost = cost2;
    if (ready) broadcastLinkState(n, cost, vns);
    else cout << " Waiting for " << n - vns.size() << " more VN(s)\n";
}

// Protocol functions

bool receiveConnect(int fd, VN &vn) {
    if(!recvAll(fd, vn.addr, 6)) return false;      // 6 as ip(4)+port(2)
    vn.fd = fd;
    return true;
}

vector<uint8_t> formLinkState(int i, int n, const vector<vector<int>> &cost, const vector<VN> &vns) {
    vector<uint8_t> msg = {0};
    // todo
    return msg;
}

void sendLinkState(int i, int n, const vector<vector<int>> &cost, const vector<VN> &vns) {
    if (vns[i].fd < 0) return;
    vector<uint8_t> msg = formLinkState(i, n, cost, vns);
    if (!sendAll(vns[i].fd, msg.data(), msg.size())) {
        cerr << "sending message to" << char('A'+i) << "failed\n";
    }
}

void broadcastLinkState(int n, const vector<vector<int>> &cost, const vector<VN> &vns) {
    for (int i = 0; i < n; i++) sendLinkState(i, n, cost, vns);
    cout << "Link-State pushed\n";
}

// VN addition and removal

void addNewVN(int srv, vector<VN> &vns, int n, const vector<vector<int>> &cost, bool &ready) {
    int c = acceptClient(srv);
    if (c < 0) return;
    VN vn;
    if (vns.size() >= MAXN || !receiveConnect(c, vn)) {
        cerr << "Rejected a connection\n";
        closeSocket(c);
        return;
    }
    vns.push_back(vn);
    int index = vns.size()-1;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, vn.addr, ip, sizeof ip);
    int port = (vn.addr[4] << 8) | vn.addr[5];
    cout << "Connected from node " << char('A'+index) << " " << ip << ":" << port << "\n";
    if(!ready && vns.size()>=n) { ready = true; broadcastLinkState(n, cost, vns); }
    else if (ready) { sendLinkState(index, n, cost, vns); }
}

void removeVN(VN &vn, int index) {
    uint8_t b;
    if(recv(vn.fd, &b, 1, 0) <= 0) {
        cout << "Node " << char('A' + index) << "disconnected\n";
        closeSocket(vn.fd);
        vn.fd = -1;
    }
}


// main

int main(int argc, char **argv) {
    
    if (argc != 2) { cerr << "usage: ./on <config-file>\n"; return 1; }
    string path = argv[1];
    int n;
    vector<vector<int>> cost;
    if(!parseConfig(path, n, cost)) { cerr << "Incorrect file format"; return 1; }

    int srv = createSocket();
    bindSocket(srv, ON_PORT);
    listenSocket(srv, 32);
    cout << "ON listening on TCP port " << ON_PORT << ", waiting for " << n << "nodes\n";

    vector<VN> vns;
    bool ready = false;

    // todo while loop for polling
    
    closeSocket(srv);
    return 0;
}