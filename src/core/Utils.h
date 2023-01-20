#pragma once

#include <string>
#include "ABY_utils/ABYconstants.h"

#include "ENCRYPTO_utils/connection.h"
#include "ENCRYPTO_utils/socket.h"
#include "ENCRYPTO_utils/crypto/crypto.h"
#include "ENCRYPTO_utils/parse_options.h"

struct CircSMPContext {
    std::string address;
    std::uint16_t port;
    e_role role;

    uint32_t circ_depth, circ_gates;
    double comm_cost, time_cost;
};

CircSMPContext read_options(int argc, char ** argv);
std::unique_ptr<CSocket> EstablishConnection(const std::string &address, uint16_t port, e_role role);
void RevealResult(std::vector<std::vector<uint32_t>> tuples, CircSMPContext context, e_sharing sharing_type = S_BOOL);
void RevealResult(std::vector<uint64_t> tuples, CircSMPContext context, e_sharing sharing_type = S_BOOL);
void RevealResult(std::vector<uint32_t> tuples, CircSMPContext context, e_sharing sharing_type = S_BOOL);
void PrintInfo(CircSMPContext context);