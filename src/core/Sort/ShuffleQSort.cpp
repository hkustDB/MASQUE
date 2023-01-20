#include "ShuffleQSort.h"

#include <random>

void ShuffleQSort(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> &vals, CircSMPContext &context) {
    auto start_time = clock();
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = keys.size();
    uint32_t nattrs = vals[0].size();
    // Random Shuffle
    std::random_device rd;
    std::mt19937 rng(rd());
    std::vector<uint32_t> indices(neles);
    std::vector<std::vector<uint32_t>> tuples(neles);
    for (auto i=0u; i<neles; ++i) {
        indices[i] = i;
    }
    shuffle(indices.begin(), indices.end(), rng);
    for (auto i=0u; i<neles; ++i) {
        tuples[i] = vals[i];
        tuples[i].push_back((uint32_t) (keys[i] >> 32));
        tuples[i].push_back((uint32_t) (keys[i] & ((1ULL << 32) - 1)));
    }
    PermutationNetwork(tuples, indices, SERVER, context.role);
    gParty.Update(context);
    gParty.Reset();
    PermutationNetwork(tuples, indices, CLIENT, context.role);
    gParty.Update(context);
    gParty.Reset();

    for (auto i=0u; i<neles; ++i) {
        keys[i] = ((uint64_t)tuples[i][nattrs] << 32) + tuples[i][nattrs + 1];
        vals[i] = {tuples[i].begin(), tuples[i].end() - 2};
    }

    // QSort
    std::vector<share*> shr_keys (neles), comptags (neles), outtags (neles);
    std::vector<bool> tags (neles);
    Circuit *bc = gParty.GetCircuit(S_BOOL);
    std::vector<std::pair<uint32_t, uint32_t>> intervals = {std::make_pair(0, neles-1)}, newintervals;

    uint32_t rounds = 0;

    while (true) {
        for (auto i=0u; i<neles; ++i) {
            shr_keys[i] = bc->PutSharedINGate(keys[i], 64);
        }

        bool flag = true;
        for (auto inter : intervals) {
            uint32_t L = inter.first, R = inter.second;
            if (L < R) {
                flag = false;
                for (auto i=L+1; i<=R; ++i) {
                    comptags[i] = new boolshare(1, bc);
                    comptags[i] = bc->PutGTGate(shr_keys[i], shr_keys[L]);
                    outtags[i] = bc->PutOUTGate(comptags[i], ALL);
                }
            }
        }
        if (flag) break;
        gParty.ExecCircuit();

        newintervals.clear();
        for (auto inter : intervals) {
            uint32_t L = inter.first, R = inter.second;
            if (L < R) {
                // std::cerr << L << ',' << R << ' ';
                for (auto i=L+1; i<=R; ++i) {
                    tags[i] = outtags[i] -> get_clear_value<bool>();
                    // std::cerr << tags[i] << ' ';
                }
                // std::cerr << std::endl;
                uint32_t i = L+1, j = R;
                while (i <= j) {
                    while (i <= j && tags[i] == false) ++i;
                    while (i <= j && tags[j] == true) --j;
                    if (i < j) {
                        std::swap(keys[i], keys[j]);
                        std::swap(vals[i], vals[j]);
                        i += 1;
                        j -= 1;
                    }
                }
                i -= 1;
                if (i != L) {
                    std::swap(keys[i], keys[L]);
                    std::swap(vals[i], vals[L]);
                }
                // std::cerr << " ==> " << L << ' ' << i << ' ' << R << std::endl;
                if (L != i) newintervals.push_back(std::make_pair(L, i-1));
                if (i != R) newintervals.push_back(std::make_pair(i+1, R));
            }
            // std::cerr << std::endl;
        }
        intervals = newintervals;
        gParty.Update(context);
        gParty.Reset();
        rounds += 1;
    }

    std::cout << "QSort Rounds = " << rounds << std::endl;

    auto end_time = clock();
    context.time_cost = 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    return;
}
