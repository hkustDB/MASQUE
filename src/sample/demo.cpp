#include "../core/Utils.h"
#include "shufflesample.h"
#include "wrsample.h"
#include "worsample.h"
#include "stratifiedsample.h"

#include <vector>

using namespace std;

void ShuffleSampleDemo(CircSMPContext context) {
    uint32_t neles = 100000;
    vector<vector<uint32_t>> tuples(neles);
    for (auto i=0U; i<neles; ++i) {
        if (context.role == SERVER) {
            tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
        } else {
            tuples[i] = {0,0,0};
        }
    }
    ShuffleSample(tuples, context);
    // RevealResult(tuples, context);
    PrintInfo(context);
}

void UniformWRSampleDemo(CircSMPContext context) {
    uint32_t neles = 10000;
    vector<vector<uint32_t>> tuples(neles), samples;
    for (auto i=0U; i<neles; ++i) {
        if (context.role == SERVER) {
            tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
        } else {
            tuples[i] = {0,0,0};
        }
    }
    UniformWRSample(tuples, samples, neles, context);
    // RevealResult(samples, context);
    PrintInfo(context);
}

void WoRSampleDemo(CircSMPContext context) {
    uint32_t neles = 1000;
    // vector<vector<uint32_t>> R(neles), S(neles), T;
    // for (auto i=0U; i<neles; ++i) {
    //     if (context.role == SERVER) {
    //         R[i] = {(i+1), (i+1)*10};
    //         S[i] = {(i+1), (i+1)*100};
    //     } else {
    //         R[i] = {0,0};
    //         S[i] = {0,0};
    //     }
    // }
    // // if (context.role == SERVER) {
    // //     R = {{1, 10}, {1, 11}, {2, 20}, {3, 30}};
    // //     S = {{1, 1}, {2,2}, {4,4}};
    // // } else {
    // //     R = {{0,0}, {0,0}, {0,0}, {0,0}};
    // //     S = {{0,0}, {0,0}, {0,0}};
    // // }
    // FKJoin(R, S, T, context);

    // vector<vector<uint32_t>> L(neles);
    // if (context.role == SERVER) {
    //     L = {{1,2}, {2,3}, {3,4}, {4,5}, {6,7}};
    // } else {
    //     L = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}};
    // }
    // PointerJumping(L, context);
    // PointerJumping(L, context);

    // GenWoRIndices(10, 5, 5, context);

    // vector<uint32_t> indices (10, 0);
    // if (context.role == SERVER) {
    //     indices = {0, 2, 4, 1, 3, 4, 3, 2, 1, 0};
    // }
    vector<vector<uint32_t>> tuples(neles), samples;
    for (auto i=0U; i<neles; ++i) {
        if (context.role == SERVER) {
            tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
        } else {
            tuples[i] = {0,0,0};
        }
    }
    // GetSamples(indices, tuples, samples, context);
    WoRSample(tuples, samples, 7 * neles / 50, 50, context);
    // RevealResult(samples, context);
    PrintInfo(context);

    // UniformWoRSample(tuples, samples, neles, context);
    // vector<vector<uint32_t>> tuples(neles), samples;
    // for (auto i=0U; i<neles; ++i) {
    //     if (context.role == SERVER) {
    //         tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
    //     } else {
    //         tuples[i] = {0,0,0};
    //     }
    // }
    // UniformWoRSample(tuples, samples, neles, context);
    // RevealResult(samples, context);
    // PrintInfo(context);

    return;
}

void StratifiedDemo(CircSMPContext context) {
    // uint32_t neles = 10;
    // vector<vector<uint32_t>> tuples(neles), samples;
    // for (auto i=0U; i<neles; ++i) {
    //     if (context.role == SERVER) {
    //         tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
    //     } else {
    //         tuples[i] = {0,0,0};
    //     }
    // }

    // vector<vector<uint32_t>> tuples;
    // vector<uint32_t> degtable;
    // if (context.role == SERVER) {
    //     tuples = {{4, 444}, {2, 22}, {2, 222}, {1, 11}, {1, 111}, {4, 44}, {1, 1111}, {3, 33}};
    // } else {
    //     tuples = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    // }
    // DegreeClassification(4, tuples, degtable, context);

    vector<uint32_t> degtable;
    vector<vector<uint32_t>> sampletable;
    uint32_t k, g, s;
    g = 4; s = 4;
    if (context.role == SERVER) {
        k = 1;
        degtable = {1, 2, 2, 3};
    } else {
        k = 0;
        degtable = {0, 0, 0, 0};
    }
    GenSampleTable(degtable, sampletable, s, k, context);
}

void GenIDsDemo(CircSMPContext context) {
    std::vector<uint32_t> ids;
    uint32_t s = 100000;
    uint32_t d = s;
    if (context.role == CLIENT) {
        d = 0;
    }
    GenRandomIDs(ids, s, d, context);
    // RevealResult(ids, context);
    PrintInfo(context);
}

void GetSamplesDemo(CircSMPContext context) {
    uint32_t n = 1000000;
    std::vector<uint32_t> indices(n);
    std::vector<std::vector<uint32_t>> tuples(n), samples;
    for (auto i=0u; i<n; ++i) {
        indices[i] = 0;
        tuples[i] = {0, 0, 0};
        if (context.role == SERVER) {
            indices[i] = rand() % n;
            tuples[i] = {(i+1), (i+1)*11, (i+1)*111};
        }
    }
    GetSamples(indices, tuples, samples, context);
    PrintInfo(context);
}

int main(int argc, char ** argv) {
    auto context = read_options(argc, argv);
    // ShuffleSampleDemo(context);
    // UniformWRSampleDemo(context);
    // WoRSampleDemo(context);
    // StratifiedDemo(context);
    // GenIDsDemo(context);
    GetSamplesDemo(context);
    return 0;
}