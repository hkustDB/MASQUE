#include "Utils.h"

#include <iostream>
#include <boost/program_options.hpp>

CircSMPContext read_options(int argc, char ** argv) {
    CircSMPContext context;

    context.address = "127.0.0.1";
    context.port = (uint16_t) 7788;
    context.role = (e_role) std::stoi(argv[1]);

    return context;
}

std::unique_ptr<CSocket> EstablishConnection(const std::string &address, uint16_t port,
                                             e_role role) {
    std::unique_ptr<CSocket> socket;
    if (role == SERVER) {
        socket = Listen(address.c_str(), port);
    } else {
        socket = Connect(address.c_str(), port);
    }
    assert(socket);
    return socket;
}

void RevealResult(std::vector<uint32_t> tuples, CircSMPContext context, e_sharing sharing_type) {
    std::unique_ptr<CSocket> sock = EstablishConnection(context.address, context.port, static_cast<e_role>(context.role));

    uint32_t neles = tuples.size();
    std::vector<uint32_t> msgs;

    if (context.role == CLIENT) {
        msgs.push_back(neles);
        for (auto i=0U; i<neles; ++i) {
            msgs.push_back(tuples[i]);
        }
        sock->Send(msgs.data(), msgs.size() * sizeof(uint32_t));
        sock->Close();
    } else {
        msgs.resize(neles + 1);
        sock->Receive(msgs.data(), msgs.size() * sizeof(uint32_t));
        sock->Close();
        assert (msgs[0] == neles);
        printf("=====================\n");
        printf("    reveal result    \n");
        for (auto i=0U; i<neles; ++i) {
                uint32_t val = msgs[i + 1];
                if (sharing_type == S_BOOL) {
                    val = val ^ tuples[i];
                } else {
                    val = val + tuples[i];
                }
                printf("%u\n", val);
        }
        printf("=====================\n");
    }
}

void RevealResult(std::vector<uint64_t> tuples, CircSMPContext context, e_sharing sharing_type) {
    std::unique_ptr<CSocket> sock = EstablishConnection(context.address, context.port, static_cast<e_role>(context.role));

    uint32_t neles = tuples.size();
    std::vector<uint64_t> msgs;

    if (context.role == CLIENT) {
        msgs.push_back(neles);
        for (auto i=0U; i<neles; ++i) {
            msgs.push_back(tuples[i]);
        }
        sock->Send(msgs.data(), msgs.size() * sizeof(uint64_t));
        sock->Close();
    } else {
        msgs.resize(neles + 1);
        sock->Receive(msgs.data(), msgs.size() * sizeof(uint64_t));
        sock->Close();
        assert (msgs[0] == neles);
        printf("=====================\n");
        printf("    reveal result    \n");
        for (auto i=0U; i<neles; ++i) {
                uint64_t val = msgs[i + 1];
                if (sharing_type == S_BOOL) {
                    val = val ^ tuples[i];
                } else {
                    val = val + tuples[i];
                }
                printf("%llu\n", val);
        }
        printf("=====================\n");
    }
}

void RevealResult(std::vector<std::vector<uint32_t>> tuples, CircSMPContext context, e_sharing sharing_type) {
    std::unique_ptr<CSocket> sock = EstablishConnection(context.address, context.port, static_cast<e_role>(context.role));

    uint32_t neles = tuples.size();
    uint32_t nattrs = tuples[0].size();
    std::vector<uint32_t> msgs;

    if (context.role == CLIENT) {
        msgs.push_back(neles);
        msgs.push_back(nattrs);
        for (auto i=0U; i<neles; ++i) {
            for (auto j=0U; j<nattrs; ++j) {
                msgs.push_back(tuples[i][j]);
            }
        }
        sock->Send(msgs.data(), msgs.size() * sizeof(uint32_t));
        sock->Close();
    } else {
        msgs.resize(nattrs * neles + 2);
        sock->Receive(msgs.data(), msgs.size() * sizeof(uint32_t));
        sock->Close();
        assert (msgs[0] == neles);
        assert (msgs[1] == nattrs);
        printf("=====================\n");
        printf("    reveal result    \n");
        for (auto i=0U; i<neles; ++i) {
            for (auto j=0U; j<nattrs; ++j) {
                uint32_t val = msgs[i * nattrs + j + 2];
                if (sharing_type == S_BOOL) {
                    val = val ^ tuples[i][j];
                } else {
                    val = val + tuples[i][j];
                }
                printf("%u ", val);
            }
            printf("\n");
        }
        printf("=====================\n");
    }
}

void PrintInfo(CircSMPContext context) {
    printf("Total Depth in Circuit = %u\n", context.circ_depth);
    printf("Total Gates in Circuit = %u\n", context.circ_gates);

    printf("Communication cost = %lf MB\n", context.comm_cost);
    printf("Running time cost = %lf s\n", context.time_cost);
}