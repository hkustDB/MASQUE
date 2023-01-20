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

void ShuffleSample(std::vector<std::vector<uint32_t>> &tuples, CircSMPContext &context);