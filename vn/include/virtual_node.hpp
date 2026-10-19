#pragma once

#include <netinet/in.h>
#include "common.hpp"
#include "neighbour_table.hpp"
#include "link_state.hpp"

class VirtualNode {
    int tcp = -1;
    int udp = -1;
    int self = -1;
    uint32_t seq = 0;
    sockaddr_in mine{};
    NeighbourTable neighbours;
    LinkState ls;

    static void printNeighbours(const std::vector<Tuple> &tups);
    Lsp makeLsp();
    void flood(const Lsp &lsp, const sockaddr_in* from);
    void publishLsp();
    bool onOracleMessage();
    void onLspMessage();
    void onTick();

public:
    void init (const char* onIp, const char *myIp, int udpPort);
    void run();

};