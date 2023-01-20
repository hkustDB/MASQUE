#include "../core/Utils.h"
#include "../core/Party.h"
#include "../core/PermutationNetwork.h"
#include "../core/Sort/SortingNetwork.h"
#include "../core/Sort/OptSortingNetwork.h"
#include "../core/Sort/YaoSortingNetwork.h"
#include "../core/Sort/ShuffleQSort.h"

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

#include <vector>

using namespace std;

void TestYaoSortNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = neles - i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles), outs;
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i] * 10000000000000ULL;
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    auto start_time = clock();
    YaoSortNet ysn;
    ysn.LoadData(keys, vals);
    ysn.ExecSortNet(context);
    ysn.OutputData(outs);
    // RevealResult(outs, context);
    auto end_time = clock();
    context.time_cost = 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    PrintInfo(context);
}

void TestOptSortNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = neles - i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles), outs;
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i] * 10000000000000ULL;
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    auto start_time = clock();
    SortNet sn;
    sn.LoadData(keys, vals);
    sn.ExecSortNet(context);
    sn.OutputData(outs);
    // RevealResult(outs, context);
    auto end_time = clock();
    context.time_cost = 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    PrintInfo(context);
}

void TestSortNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = neles - i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles);
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i] * 10000000000000ULL;
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    SortingNetwork(keys, vals, context.role);
    // for (auto i=0U; i<vals.size(); ++i) {
    //     for (auto j:vals[i]) {
    //         cout << j << ',' ;
    //     }
    //     cout << endl;
    // }
    // cout << endl;
    PrintInfo(context);
}

void TestShuffleQSort(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles);
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i];
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }

    // RevealResult(vals, context);

    ShuffleQSort(keys, vals, context);
    // RevealResult(vals, context);
    PrintInfo(context);
}

int main(int argc, char ** argv) {
    auto context = read_options(argc, argv);

    // TestSortNet(context);
    TestOptSortNet(context);
    // TestYaoSortNet(context);
    // TestShuffleQSort(context);

    return 0;
}