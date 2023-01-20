#pragma once

#include <string>
#include "aby/abyparty.h"
#include "circuit/circuit.h"
#include "sharing/sharing.h"
#include "Utils.h"

class Party {
public:
    void Init(std::string address_, uint16_t port_, e_role role_);
    std::string GetAddress() {return address;}
    std::uint16_t GetPort() {return port;}
    e_role GetRole() {return role;}
    Circuit* GetCircuit(e_sharing sharingType);
    void ExecCircuit();
    void Reset();
    void Update(CircSMPContext &context);
    uint32_t GetGates();

private:
    std::string address;
    std::uint16_t port;
    e_role role;
    
    ABYParty *abyparty;
	// Circuit *ac, *bc, *yc;
    
    uint64_t circ_depth, circ_gates, comm_cost;
};

extern Party gParty;
