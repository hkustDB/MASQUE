#pragma once

#include "../Party.h"
#include "../Utils.h"
#include "../PermutationNetwork.h"

#include "sharing/sharing.h"
#include "circuit/booleancircuits.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "circuit/circuit.h"
#include "aby/abyparty.h"

void ShuffleQSort(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> &vals, CircSMPContext &context);