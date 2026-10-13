#pragma once

#include<cstdint>
#include<vector>
#include<netinet/in.h>


const int ON_PORT   = 5000;     // Port of Oracle Node
const int MAXN      = 26;       // Max Nodes allowed
const int ENTRY_SZ  = 11;       // Size of tuple: letter(1) + ip(4) + port(2) + cost(4)
const int HDR       = 6;        // Size of LSP header: origin(1) + seq(4) + count(1)
const int PERIOD    = 10;       // Time period between each LSP 


struct Tuple {
    char        id;
    uint32_t    ip;
    uint16_t    port;
    uint32_t    cost;
};


struct LSP {
    int                 origin = -1;
    uint32_t            seq = 0;
    std::vector<Tuple>  tuples;
};


struct Neighbour {
    char        id;
    sockaddr_in addr;
    uint32_t    cost;
};