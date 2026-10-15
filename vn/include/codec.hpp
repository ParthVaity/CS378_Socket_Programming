#pragma once

#include"common.hpp"

std::vector<uint8_t> encodeConnect(const sockaddr_in& mine);
std::vector<uint8_t> encodeLSP(const Lsp& lsp);
bool decodeLSP(const uint8_t* buf, int len, Lsp& lsp);
bool recvLinkState(int tcp, std::vector<Tuple>& out);