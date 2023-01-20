#include "../core/Utils.h"
#include "shufflesample.h"

#include <algorithm>
#include <random>

using namespace std;

void ShuffleSample(std::vector<std::vector<uint32_t>> &tuples, CircSMPContext &context) {
    auto start_time = clock();
    gParty.Init(context.address, context.port, context.role);

    uint32_t neles = tuples.size();
    std::random_device rd;
    std::mt19937 seed(rd());
    std::vector<uint32_t> indices (neles);
    for (auto i=0U; i<neles; ++i) {indices[i] = i;}
    std::shuffle(indices.begin(), indices.end(), seed);

    PermutationNetwork(tuples, indices, SERVER, context.role);
    gParty.Update(context);
    gParty.Reset();
    PermutationNetwork(tuples, indices, CLIENT, context.role);
    gParty.Update(context);

    auto end_time = clock();
    context.time_cost += 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    return;
}