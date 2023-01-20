#pragma once 

#include "../core/Utils.h"
#include "../core/Party.h"
#include "../core/PermutationNetwork.h"
#include "../core/Sort/SortingNetwork.h"

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

// sample_size = number_of_batches * batch_size [total number of generated WR samples]
void UniformWRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t sample_size, CircSMPContext &context);