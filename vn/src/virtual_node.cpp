#include "virtual_node.hpp"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sys/select.h>
#include <sys/socket.h>
#include "socket_util.hpp"
#include "codec.hpp"

using namespace std;

void VirtualNode::printNeighbours (const std::vector<Tuple> &tups) {
    for (const auto &tup : tups) {
        printf("id: %c, ip: %u, port: %u, cost: %u\n", tup.id, tup.ip, tup.port, tup.cost);
    }
}

Lsp VirtualNode::makeLsp() {
    Lsp lsp;
    lsp.origin = self;
    lsp.seq = seq++;
    lsp.tuples.push_back({(char)('A'+self), mine.sin_addr.s_addr, mine.sin_port, 0});
    for(auto& v:neighbours.allNeighbours()) {
        const Neighbour& n = v.second;
        lsp.tuples.push_back({n.id, n.addr.sin_addr.s_addr, n.addr.sin_port, n.cost});
    }
    return lsp;
}

void VirtualNode::flood(const Lsp &lsp, const sockaddr_in* from) {
    vector<uint8_t> buff = encodeLSP(lsp);
    for (auto& v:neighbours.allNeighbours()) {
        const Neighbour& n = v.second;
        if (from && NeighbourTable::sameAddr(n.addr, *from)) continue;
        for (int t=0; t<2; t++)  sendto(udp, buff.data(), buff.size(), 0, (const sockaddr*)&n.addr, sizeof(n.addr));
    }
}

void VirtualNode::publishLsp() {
    Lsp lsp = makeLsp();
    ls.update(lsp);
    flood(lsp, nullptr);
}

bool VirtualNode::onOracleMessage() {
    vector<Tuple> tups;
    if (!recvLinkState(tcp, tups)) return false;
    int id = -1;
    for (auto& tup:tups) if (tup.cost == 0) id = tup.id - 'A';
    if (id == -1) return false;
    self = id;
    printNeighbours(tups);
    neighbours.remake(tups);
    publishLsp();
    return true;
}

void VirtualNode::onLspMessage() {
    uint8_t buff[HDR + MAXN * ENTRY_SZ];
    sockaddr_in from{};
    socklen_t fromlen = sizeof(from);
    int n = recvfrom(udp, buff, sizeof(buff), 0, (sockaddr*)&from, &fromlen);
    Lsp lsp;
    if (n <= 0 || !decodeLSP(buff, n, lsp)) return;
    if (lsp.origin == self) return;
    if (!ls.update(lsp)) return;
    if (self >= 0) flood(lsp, &from);
}

void VirtualNode::onTick() {
    publishLsp();
    ls.print();
}

void VirtualNode::init(const char* onIp, const char *myIp, int udpPort) {
    mine = makeAddr(myIp, udpPort);
    udp = createUDPSocket();
    bindSocket(udp, mine);
    sockaddr_in onAddr = makeAddr(onIp, ON_PORT);
    tcp = createTCPSocket();
    connectToServer(tcp, onAddr);
    vector<uint8_t> buff = encodeConnect(mine);
    if (!sendAll(tcp, buff.data(), buff.size())) {
        cerr << "Failed to send connect message to Oracle Node" << endl;
        exit(1);
    }
}

void VirtualNode::run() {
    time_t lastTick = time(nullptr);
    for(;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(tcp, &rfds);
        FD_SET(udp, &rfds);
        timeval tv{1, 0};
        if (select(max(tcp, udp)+1, &rfds, nullptr, nullptr, &tv) < 0) {
            perror("select");
            break;
        }
        if (FD_ISSET(tcp, &rfds) && !onOracleMessage()) {
            cerr << "Connection to ON lost" << endl;
            break;
        }
        if (FD_ISSET(udp, &rfds)) onLspMessage();
        time_t now = time(nullptr);
        if (self >= 0 && (now - lastTick >= PERIOD)) {
            lastTick = now;
            onTick();
        }
    }
    closeSocket(tcp);
    closeSocket(udp);
}