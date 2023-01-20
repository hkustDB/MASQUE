#include "Party.h"

Party gParty;

void Party::Init(std::string address_, uint16_t port_, e_role role_) {
    address = address_;
    port = port_;
    role = role_;

    if (abyparty != NULL) {
        delete abyparty;
    }
    
    abyparty = new ABYParty(role, address, port, LT, 32, 1); // Default setting: bit length = 32; thread = 1;
    abyparty->ConnectAndBaseOTs();

    circ_depth = circ_gates = comm_cost = 0;
}

void Party::Update(CircSMPContext &context) {
    context.circ_depth += circ_depth;
    context.circ_gates += circ_gates;
    context.comm_cost += 1.0 * comm_cost / 1024.0 / 1024.0; // MB
    circ_depth = circ_gates = comm_cost = 0;
}

void Party::Reset() {
    abyparty->Reset();
    circ_depth = circ_gates = comm_cost = 0;
}

Circuit* Party::GetCircuit(e_sharing sharingType) {
    std::vector<Sharing*> &sharings = abyparty->GetSharings();
	return sharings[sharingType]->GetCircuitBuildRoutine();
}

uint32_t Party::GetGates() {
    return abyparty->GetTotalGates();
}

void Party::ExecCircuit() {
    abyparty -> ExecCircuit();

    uint32_t circcomm = abyparty->GetReceivedData(P_SETUP) + abyparty->GetSentData(P_SETUP);
    circcomm += abyparty->GetReceivedData(P_ONLINE) + abyparty->GetSentData(P_ONLINE);
    comm_cost += circcomm;

    circ_depth += abyparty->GetTotalDepth();
    circ_gates += abyparty->GetTotalGates();

    // std::cerr << "\n==========================\n";
    // std::cerr << "Current Circuit Infomation: \n";
    // std::cerr << "  depth = " << abyparty->GetTotalDepth() << std::endl;
    // std::cerr << "  gates = " << abyparty->GetTotalGates() << std::endl;
    // std::cerr << "  comm = " << circcomm << std::endl;
    // // std::cerr << "time = " << abyparty->GetTiming(P_SETUP) + abyparty->GetTiming(P_ONLINE) << std::endl;
    // std::cerr << "=========================\n";
}