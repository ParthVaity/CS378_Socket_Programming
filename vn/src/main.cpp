#include <iostream>
#include <cstdlib>
#include "virtual_node.hpp"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./vn <ON_IP> <MY_IP> <UDP_PORT>\n";
        return 1;
    }
    VirtualNode vn;
    vn.init(argv[1], argv[2], std::atoi(argv[3]));
    vn.run();
    return 0;
}