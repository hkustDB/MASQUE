#include "wrsample.h"

#include <random>
#include <cmath>

void UniformWRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t sample_size, CircSMPContext &context) {
    auto start_time = clock();
    gParty.Init(context.address, context.port, context.role);

    uint32_t neles = tuples.size();
    uint32_t nattrs = tuples[0].size();

    std::random_device rd;
    std::mt19937 sd(rd());
    std::uniform_int_distribution<int> unifgen(0, neles - 1);
    std::vector<uint32_t> indices (sample_size);
    for (auto i=0U; i<sample_size; ++i) { // here we just simulate the shared input of randomness.
        indices[i] = unifgen(sd); // generating shared indices with modular neles
        if (context.role == CLIENT) {
            indices[i] = 0;
        }
    }
    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

    std::cerr << "initial gates " << gParty.GetGates() << std::endl;

    std::vector<share*> sample_ids (sample_size);
    share* zero = bc->PutINGate((uint32_t)0, 32, SERVER);
    share* one = bc->PutINGate((uint32_t)1, 32, SERVER);
    share* shr_neles = bc->PutINGate(neles, 32, SERVER);
    share* neles_1 = bc->PutINGate(neles-1, 32, SERVER);
    // Convert share from modular `neles` to modular `2^32`
    for (auto i=0U; i<sample_size; ++i) {
        sample_ids[i] = ac->PutSharedINGate(indices[i], 32);
        sample_ids[i] = bc->PutA2BGate(sample_ids[i], yc);
        
        // share* selbit = bc->PutGTGate(sample_ids[i], neles_1); // Check if indices[i] >= neles
        // share* minus = bc->PutMUXGate(shr_neles, zero, selbit);
        // sample_ids[i] = bc->PutSUBGate(sample_ids[i], minus); // If true, sample_id should be minus neles. We assume neles < 2^31

        // std::string outargu = "sample id " + std::to_string(i);
        // bc->PutPrintValueGate(sample_ids[i], outargu.c_str());
    }

    // Generate sorting tuples
    // keys - sorting key
    // values - [ data value, original ID in sample ]
    uint32_t nsort = sample_size + neles;
    std::vector<share*> keys (nsort), values (nsort);
    std::vector<uint32_t> empty (nattrs + 1, 0), tuple;
        // sample index
    for (auto i=0U; i<sample_size; ++i) {
        keys[i] = bc->PutADDGate(sample_ids[i], sample_ids[i]);
        keys[i] = bc->PutADDGate(keys[i], one);
        if (context.role == SERVER) {
            empty[nattrs] = i+1; // ID from 1 to sample_size, NOT START FROM 0 !!!
        }
        values[i] = ac->PutSharedSIMDINGate(nattrs + 1, (uint32_t*)empty.data(), 32);
        values[i] = bc->PutA2BGate(values[i], yc);
    }
        // data set
    for (auto i=sample_size; i<nsort; ++i) {
        tuple = tuples[i - sample_size];
        tuple.push_back(0);
        keys[i] = bc->PutINGate((i - sample_size) << 1, 32, SERVER);
        values[i] = ac->PutSharedSIMDINGate(nattrs + 1, (uint32_t*)tuple.data(), 32);
        values[i] = bc->PutA2BGate(values[i], yc);
    }

    uint32_t pow2neles = (uint32_t)pow(2, ceil(log2(nsort)));
    keys.resize(pow2neles);
    values.resize(pow2neles);
    for (auto i=nsort; i<pow2neles; ++i) {
        keys[i] = bc->PutINGate((uint32_t)-1, 32, SERVER);
        values[i] = ac->PutSIMDINGate(nattrs + 1, (uint32_t*)empty.data(), 32, SERVER);
    }

    std::cerr << "prepare sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " before sort key");
    //     bc->PutPrintValueGate(values[i], " before sort value");
    // }

    GenBitonicSort(keys, values, 0, pow2neles, TRUE, bc);

    std::cerr << "after sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], "key");
    //     bc->PutPrintValueGate(values[i], "value");
    // }

    std::vector<share*> tags (nsort);
    uint32_t posids[1] = {nattrs};
    for (auto i=0U; i<nsort; ++i) {
        // bc->PutPrintValueGate(keys[i], "old key");
        keys[i] = bc->PutSubsetGate(values[i], posids, 1);
        // bc->PutPrintValueGate(values[i], "values");
        // bc->PutPrintValueGate(keys[i], "new key");
        tags[i] = bc->PutEQGate(keys[i], zero);
    }
    for (auto i=nsort; i<pow2neles; ++i); // keys = UINT_MAX

    // Generate Prefix Sum Gates
    uint32_t logn = (uint32_t) floor(log2(nsort));
    std::vector< std::vector< share* >> level_tags(2 * logn + 3), level_vals(2 * logn + 3);
    
    uint32_t l = 0;
    level_tags[0] = tags;
    level_vals[0] = values;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(level_tags[0][i], "=== tags === ");
    //     bc->PutPrintValueGate(level_vals[0][i], "=== values === ");
    // }

    // for (auto i=0; i<=logn; ++i) {
    //     level_tags[i+1].resize(nsort);
    //     level_vals[i+1].resize(nsort);
    //     uint32_t jump = 1 << i;
    //     for (auto j=0; j<nsort; ++j) {
    //         if (j < jump) {
    //             level_tags[i+1][j] = level_tags[i][j];
    //             level_vals[i+1][j] = level_vals[i][j];
    //         } else {
    //             share* selbit = level_tags[i][j];
    //             share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
    //             level_tags[i+1][j] = bc->PutMUXGate(level_tags[i][j], level_tags[i][j-jump], selbit);
    //             level_vals[i+1][j] = bc->PutMUXGate(level_vals[i][j], level_vals[i][j-jump], selbits);
    //         }
    //     }
    // }

    for (auto i=0U; i<=logn; ++i) {
        l += 1;
        level_tags[l].resize(nsort);
        level_vals[l].resize(nsort);
        uint32_t jump = 1 << i;
        for (auto j=0U; j<nsort; ++j) {
            if ((j+1) % (2 * jump) == 0) {
                // std::cerr << j-jump << ' ' << j << std::endl;
                share* selbit = level_tags[l-1][j];
                share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
                level_tags[l][j] = bc->PutMUXGate(level_tags[l-1][j], level_tags[l-1][j-jump], selbit);
                level_vals[l][j] = bc->PutMUXGate(level_vals[l-1][j], level_vals[l-1][j-jump], selbits);
            } else {
                level_tags[l][j] = level_tags[l-1][j];
                level_vals[l][j] = level_vals[l-1][j];
            }
        }
    }
    for (int i=logn; i>=0; --i) {
        l += 1;
        level_tags[l].resize(nsort);
        level_vals[l].resize(nsort);
        uint32_t jump = 1 << i;
        for (auto j=0U; j<nsort; ++j) {
            if (j >= jump && (j+1 - jump) % (2 * jump) == 0) {
                // std::cerr << j-jump << ' ' << j << std::endl;
                share* selbit = level_tags[l-1][j];
                share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
                level_tags[l][j] = bc->PutMUXGate(level_tags[l-1][j], level_tags[l-1][j-jump], selbit);
                level_vals[l][j] = bc->PutMUXGate(level_vals[l-1][j], level_vals[l-1][j-jump], selbits);
            } else {
                level_tags[l][j] = level_tags[l-1][j];
                level_vals[l][j] = level_vals[l-1][j];
            }
        }
    }

    for (auto i=0U; i<nsort; ++i) {
        values[i] = level_vals[l][i];
    }

    std::cerr << "prefix sum gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " second round sort keys === ");
    //     bc->PutPrintValueGate(values[i], " second round sort values === ");
    // }

    GenBitonicSort(keys, values, 0, pow2neles, TRUE, bc);


    std::cerr << "Output sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " after second round sort keys === ");
    //     bc->PutPrintValueGate(values[i], " after second round sort values === ");
    // }

    for (auto i=neles; i<nsort; ++i) {
        values[i] = bc -> PutSharedOUTGate(values[i]);
    }

    std::cerr << "Final gates " << gParty.GetGates() << std::endl;

    std::cerr << "execute circuit now ..." << std::endl;
    gParty.ExecCircuit();
    gParty.Update(context);

    uint32_t nvals, bitlen, *revtuple;
    samples.resize(sample_size);
    for (auto i=0U; i<sample_size; ++i) {
        values[neles + i] -> get_clear_value_vec(&revtuple, &bitlen, &nvals);
        assert(nvals == nattrs + 1);
        assert(bitlen == 32);
        samples[i].resize(nattrs);
        for (auto j=0U; j<nattrs; ++j) {
            samples[i][j] = revtuple[j];
        }
    }

    auto end_time = clock();
    context.time_cost += 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    return;
}