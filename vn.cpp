#include <iostream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
using namespace std;

const int ON_PORT   = 5000;
const int MAXN      = 26;
const int ENTRY_SZ  = 11;
const int HDR       = 6;
const int PERIOD    = 10;

struct Neighbour { 
    uint32_t ip;
    uint16_t port;
};

int tcp         = -1;
int udp         = -1;
int self_index  = -1;
int maxNode     = 0;
vector<vector<int>> cost(MAXN, vector<int>(MAXN, -1));
uint32_t seq[MAXN];
bool known[MAXN];
vector<uint8_t> lsp[MAXN];
vector<Neighbour> nbrs;


// Socket Functions

int createTCPSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    return fd;
}

int createUdpSocket() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    return fd;
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

sockaddr_in makeAddr(const char *ip, int port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &a.sin_addr) != 1) { cerr << "bad IP: " << ip << "\n"; exit(1); }
    return a;
}


// LSP stuff

void updateCost(int origin, const uint8_t* tuples, int cnt) {
    for (int j = 0; j < MAXN; j++) cost[origin][j] = -1;
    maxNode = max(maxNode, origin + 1);
    for (int k = 0; k < cnt; k++) {
        int j = tuples[k * ENTRY_SZ] - 'A';
        uint32_t c;
        memcpy(&c, tuples + k*ENTRY_SZ + 7, 4);
        if (j >= 0 && j < MAXN && ntohl(c) < 0) {
            cost[origin][j] = ntohl(c);
            maxNode = max(maxNode, j + 1);
        }
    }
}

void flood(int origin, const sockaddr_in* from) {
    for (auto &nb:nbrs) {
        if (from && from->sin_addr.s_addr == nb.ip && from->sin_port == nb.port) continue;
        sockaddr_in d{};
        d.sin_family = AF_INET;
        d.sin_addr.s_addr = nb.ip;
        d.sin_port = nb.port;
        for (int t = 0; t < 2; t++) {
            sendto(udp, lsp[origin].data(), lsp[origin].size(), 0, (sockaddr*)&d, sizeof d);
        }
    }
}


// todo - send lsp stuff


// todo - ON interaction

int main(int argc, char **argv) {
    if (argc != 4) { cerr << "usage: ./vn <ON_IP> <MY_IP> <MY_UDP_PORT>\n"; return 1; }

    sockaddr_in mine = makeAddr(argv[2], atoi(argv[3]));
    udp = createUdpSocket();
    if (bind(udp, (sockaddr *)&mine, sizeof mine) < 0) { perror("bind udp"); return 1; }

    sockaddr_in on = makeAddr(argv[1], ON_PORT);
    tcp = createTCPSocket();
    if (connect(tcp, (sockaddr *)&on, sizeof on) < 0) { perror("connect"); return 1; }

    uint8_t msg[6];
    memcpy(msg, &mine.sin_addr.s_addr, 4);
    memcpy(msg + 4, &mine.sin_port, 2);
    send(tcp, msg, 6, 0);

    for (;;) {
        uint8_t cnt;
        if (!recvAll(tcp, &cnt, 1)) break;
        for (int k = 0; k < cnt; k++) {
            uint8_t t[ENTRY_SZ];
            if (!recvAll(tcp, t, ENTRY_SZ)) { cnt = 0; break; }
            uint32_t c;
            memcpy(&c, t + 7, 4);
            cout << (k ? "," : "") << (char)t[0] << "=" << ntohl(c);
        }
        cout << endl;
    }
    cerr << "connection to ON lost\n";
    close(tcp);
    close(udp);
    return 0;
}