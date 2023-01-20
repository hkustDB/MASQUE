#include "../core/Utils.h"
#include "../core/Party.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "../core/Sort/ShuffleQSort.h"

using namespace std;

void LoadData(string filename, vector<vector<uint32_t>> &tuples) {
    freopen(filename.c_str(), "r", stdin);
	std::ios::sync_with_stdio(false);
	string str;
	while (getline(cin, str)) {
		char *token = strtok(str.data(), " ");
		vector<uint32_t> tuple = {};
		while (token != NULL) {
			tuple.push_back(atoi(token));
			token = strtok(NULL, " ");
		}
        tuples.push_back(tuple);
	}
	cerr << "Load " << tuples.size() << 'X' << tuples[0].size() << " data from file " << filename << endl;
    return;
}

/*
Q1 lineitem data has attributes:
(l_returnflag, l_linestatus, l_quantity, l_extendedprice, l_discount, l_tax, 1)

*** CALCULATION *** 
1. sum( val[2] ),
2. sum( val[3] ),
3. (100 - val[4]) as a
4. (100 + val[5]) as b
5. sum( val[3] * a)
6. sum( val[3] * a * b)
7. sum( val[5] )
8. sum( val[6] )
*** CONDITION ***
1. val[0] = 'N' // 78
2. val[1] = 'O' // 79
*/

void GenQ1Circuit(vector<vector<uint32_t>> tuples, uint32_t nsamp, CircSMPContext context) {
	auto start = clock();

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	gParty.Init(context.address, context.port, context.role);
	Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

	vector<vector<share*>> vals (nsamp);
	vector<vector<share*>> sum (nsamp);

	// Read (boolean-)shared data
	for (auto i=0u; i<nsamp; ++i) {
		vals[i].resize(nattrs);
		for (auto j=0u; j<nattrs; ++j) {
			vals[i][j] = bc->PutSharedINGate(tuples[i][j], 32);
			vals[i][j] = yc->PutB2YGate(vals[i][j]);
		}
	}

	share* shrN = yc->PutINGate((uint32_t)('N'), 32, SERVER);
	share* shrO = yc->PutINGate((uint32_t)('O'), 32, SERVER);
	share* zero = yc->PutINGate(0u, 32, SERVER);
	share* hundred = yc->PutINGate(100u, 32, SERVER);
	for (auto i=0u; i<nsamp; ++i) {
		sum[i].resize(6);
		share* selbit1 = yc->PutEQGate(vals[i][0], shrN);
		share* selbit2 = yc->PutEQGate(vals[i][1], shrO);
		share* selbit = yc->PutANDGate(selbit1, selbit2);
		if (i == 0u) {
			for (auto j=0; j<6; ++j) {
				sum[i][j] = zero;
			}
		} else {
			for (auto j=0; j<6; ++j) {
				sum[i][j] = sum[i-1][j];
			}
		}

		// sum( val[2] )
		sum[i][0] = yc->PutADDGate(sum[i][0], yc->PutMUXGate(vals[i][2], zero, selbit));
		// sum( val[3] )
		sum[i][1] = yc->PutADDGate(sum[i][1], yc->PutMUXGate(vals[i][3], zero, selbit));
		share* a = yc->PutSUBGate(hundred, vals[i][4]);
		share* b = yc->PutADDGate(hundred, vals[i][5]);
		share* v5 = yc->PutMULGate(vals[i][3], a);
		// sum( val[3] * a )
		sum[i][2] = yc->PutADDGate(sum[i][2], yc->PutMUXGate(v5, zero, selbit));
		share* v6 = yc->PutMULGate(v5, b);
		// sum( val[3] * a * b)
		sum[i][3] = yc->PutADDGate(sum[i][3], yc->PutMUXGate(v6, zero, selbit));
		// sum( val[5] )
		sum[i][4] = yc->PutADDGate(sum[i][4], yc->PutMUXGate(vals[i][5], zero, selbit));
		// sum( val[6] )
		sum[i][5] = yc->PutADDGate(sum[i][5], yc->PutMUXGate(vals[i][6], zero, selbit));
	}

	gParty.ExecCircuit();
	gParty.Update(context);
	gParty.Reset();

	auto end = clock();
	context.time_cost = 1.0 * (end - start) / CLOCKS_PER_SEC;

	std::cout << "In Sample Size = " << nsamp << " Circuit Cost" << endl;
	PrintInfo(context);
}

void GenQ1SSCircuit(vector<vector<uint32_t>> tuples, uint32_t nsamp, CircSMPContext context) {
	auto start = clock();

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	gParty.Init(context.address, context.port, context.role);
	Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

	vector<vector<share*>> vals (nsamp);
	vector<vector<share*>> sum (nsamp);

	// Read (boolean-)shared data
	for (auto i=0u; i<nsamp; ++i) {
		vals[i].resize(nattrs);
		for (auto j=0u; j<nattrs; ++j) {
			vals[i][j] = bc->PutSharedINGate(tuples[i][j], 32);
			vals[i][j] = yc->PutB2YGate(vals[i][j]);
		}
	}

	share* shrN = yc->PutINGate((uint32_t)('N'), 32, SERVER);
	share* shrO = yc->PutINGate((uint32_t)('O'), 32, SERVER);
	share* zero = yc->PutINGate(0u, 32, SERVER);
	share* hundred = yc->PutINGate(100u, 32, SERVER);
	for (auto i=0u; i<nsamp; ++i) {
		sum[i].resize(6);
		share *selbit1, *selbit2;
		if (i == 0u) {
			for (auto j=0; j<6; ++j) {
				sum[i][j] = zero;
			}
			selbit1 = yc->PutEQGate(zero, zero);
			selbit2 = yc->PutEQGate(zero, zero);
		} else {
			for (auto j=0; j<6; ++j) {
				sum[i][j] = sum[i-1][j];
			}
			selbit1 = yc->PutEQGate(vals[i][0], vals[i-1][0]);
			selbit2 = yc->PutEQGate(vals[i][1], vals[i-1][1]);
		}
		share* selbit = yc->PutANDGate(selbit1, selbit2);

		// sum( val[2] )
		sum[i][0] = yc->PutADDGate(sum[i][0], yc->PutMUXGate(vals[i][2], zero, selbit));
		// sum( val[3] )
		sum[i][1] = yc->PutADDGate(sum[i][1], yc->PutMUXGate(vals[i][3], zero, selbit));
		share* a = yc->PutSUBGate(hundred, vals[i][4]);
		share* b = yc->PutADDGate(hundred, vals[i][5]);
		share* v5 = yc->PutMULGate(vals[i][3], a);
		// sum( val[3] * a )
		sum[i][2] = yc->PutADDGate(sum[i][2], yc->PutMUXGate(v5, zero, selbit));
		share* v6 = yc->PutMULGate(v5, b);
		// sum( val[3] * a * b)
		sum[i][3] = yc->PutADDGate(sum[i][3], yc->PutMUXGate(v6, zero, selbit));
		// sum( val[5] )
		sum[i][4] = yc->PutADDGate(sum[i][4], yc->PutMUXGate(vals[i][5], zero, selbit));
		// sum( val[6] )
		sum[i][5] = yc->PutADDGate(sum[i][5], yc->PutMUXGate(vals[i][6], zero, selbit));
	}

	gParty.ExecCircuit();
	gParty.Update(context);
	gParty.Reset();

	auto end = clock();
	context.time_cost = 1.0 * (end - start) / CLOCKS_PER_SEC;

	std::cout << "In Sample Size = " << nsamp << " Circuit Cost" << endl;
	PrintInfo(context);
	std::cout << endl;
}

void Q1Demo(CircSMPContext context) {
    vector<vector<uint32_t>> tuples;
    
	string filename = "../data/lineitem" + to_string(context.role) + ".shrdat";
    LoadData(filename, tuples);

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	vector<uint32_t> nsamps = {50, 100, 500, 1000, 5000, 10000, 50000, neles};

	vector<uint64_t> keys(neles);
	for (auto i=0u; i<neles; ++i) {
		keys[i] = (context.role == CLIENT) ? 0 : neles - i;
	}
	std::cerr << "Sort " << keys.size() << ',' << tuples.size() << " elements" << endl; 
	context.time_cost = 0;
	context.comm_cost = 0;
	ShuffleQSort(keys, tuples, context);
	PrintInfo(context);

	nattrs += 1; // add a counter for "COUNT"
	for (auto i=0u; i<neles; ++i) {
		tuples[i].push_back((uint32_t)context.role); // 1 and 0 respectively
	}
	
	for (auto nsamp : nsamps) {
		GenQ1SSCircuit(tuples, nsamp, context);
	}
}


/*
q8 data has attributes:
(year, extendedprice, discount, country)

*** CALCULATION *** 
1. (100 - val[2]) as a
2. val[1] * a as b
3. (val[3] == 1) as c
4. sum(b)
5. sum(b * c)
*** CONDITION ***
1. val[i][0] == val[i-1][0]
*/


void GenQ8Circ(vector<vector<uint32_t>> tuples, uint32_t nsamp, CircSMPContext context) {
	auto start = clock();

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	gParty.Init(context.address, context.port, context.role);
	Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

	vector<vector<share*>> vals (nsamp);
	vector<vector<share*>> sum (nsamp);

	// Read (boolean-)shared data
	for (auto i=0u; i<nsamp; ++i) {
		vals[i].resize(nattrs);
		for (auto j=0u; j<nattrs; ++j) {
			vals[i][j] = bc->PutSharedINGate(tuples[i][j], 32);
			vals[i][j] = yc->PutB2YGate(vals[i][j]);
		}
	}

	share* shrN = yc->PutINGate((uint32_t)('N'), 32, SERVER);
	share* shrO = yc->PutINGate((uint32_t)('O'), 32, SERVER);
	share* zero = yc->PutINGate(0u, 32, SERVER);
	share* hundred = yc->PutINGate(100u, 32, SERVER);
	share* one = yc->PutINGate(1u, 32, SERVER);
	for (auto i=0u; i<nsamp; ++i) {
		sum[i].resize(2);
		share *selbit;
		if (i == 0u) {
			for (auto j=0; j<2; ++j) {
				sum[i][j] = zero;
			}
			selbit = yc->PutEQGate(zero, zero);
		} else {
			for (auto j=0; j<2; ++j) {
				sum[i][j] = sum[i-1][j];
			}
			selbit = yc->PutEQGate(vals[i][0], vals[i-1][0]);
		}
		// (100 - val[2]) as a
		share* a = yc->PutSUBGate(hundred, vals[i][2]);
		// val[1] * a as b
		share* b = yc->PutMULGate(vals[i][1], a);
		b = yc->PutMUXGate(b, zero, selbit);
		share* c = yc->PutEQGate(vals[i][3], one);
		
		sum[i][0] = yc->PutADDGate(sum[i][0], b);
		sum[i][1] = yc->PutADDGate(sum[i][1], yc->PutMUXGate(b, zero, c));
	}
	gParty.ExecCircuit();
	gParty.Update(context);
	gParty.Reset();

	auto end = clock();
	context.time_cost = 1.0 * (end - start) / CLOCKS_PER_SEC;

	std::cout << "In Sample Size = " << nsamp << " Circuit Cost" << endl;
	PrintInfo(context);
	std::cout << endl;
}

void Q8Demo(CircSMPContext context) {
    vector<vector<uint32_t>> tuples;
    
	string filename = "../data/q8" + to_string(context.role) + ".shrdat";
    LoadData(filename, tuples);

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	vector<uint32_t> nsamps = {50, 100, 500, 1000, 5000, 10000, 50000, neles};
	// vector<uint32_t> nsamps = {neles};

	vector<uint64_t> keys(neles);
	for (auto i=0u; i<neles; ++i) {
		keys[i] = (context.role == CLIENT) ? 0 : neles - i;
	}
	std::cout << "Sort " << keys.size() << ',' << tuples.size() << " elements" << endl; 
	context.time_cost = 0;
	context.comm_cost = 0;
	ShuffleQSort(keys, tuples, context);
	PrintInfo(context);
	context.time_cost = 0;
	context.comm_cost = 0;
	
	for (auto nsamp : nsamps) {
		GenQ8Circ(tuples, nsamp, context);
	}
}

/*
q9 data has attributes:
(nation, extendedprice, discount, supplycost, quantity)

*** CALCULATION *** 
1. (100 - val[2]) as a
2. val[1] * a as b
3. val[3] * val[4] as c
4. b - c as d
5. sum(d)
*** CONDITION ***
1. val[i][0] == val[i-1][0]
*/

void GenQ9Circ(vector<vector<uint32_t>> tuples, uint32_t nsamp, CircSMPContext context) {
	auto start = clock();

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	gParty.Init(context.address, context.port, context.role);
	Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

	vector<vector<share*>> vals (nsamp);
	vector<vector<share*>> sum (nsamp);

	// Read (boolean-)shared data
	for (auto i=0u; i<nsamp; ++i) {
		vals[i].resize(nattrs);
		for (auto j=0u; j<nattrs; ++j) {
			vals[i][j] = bc->PutSharedINGate(tuples[i][j], 32);
			vals[i][j] = yc->PutB2YGate(vals[i][j]);
		}
	}

	share* shrN = yc->PutINGate((uint32_t)('N'), 32, SERVER);
	share* shrO = yc->PutINGate((uint32_t)('O'), 32, SERVER);
	share* zero = yc->PutINGate(0u, 32, SERVER);
	share* hundred = yc->PutINGate(100u, 32, SERVER);
	share* one = yc->PutINGate(1u, 32, SERVER);
	for (auto i=0u; i<nsamp; ++i) {
		sum[i].resize(1);
		share *selbit;
		if (i == 0u) {
			for (auto j=0; j<1; ++j) {
				sum[i][j] = zero;
			}
			selbit = yc->PutEQGate(zero, zero);
		} else {
			for (auto j=0; j<1; ++j) {
				sum[i][j] = sum[i-1][j];
			}
			selbit = yc->PutEQGate(vals[i][0], vals[i-1][0]);
		}
		// (100 - val[2]) as a
		share* a = yc->PutSUBGate(hundred, vals[i][2]);
		// val[1] * a as b
		share* b = yc->PutMULGate(vals[i][1], a);
		b = yc->PutMUXGate(b, zero, selbit);
		share* c = yc->PutMULGate(vals[i][3], vals[i][4]);
		share* d = yc->PutSUBGate(b, c);
		sum[i][0] = yc->PutADDGate(sum[i][0], yc->PutMUXGate(d, zero, selbit));
	}
	gParty.ExecCircuit();
	gParty.Update(context);
	gParty.Reset();

	auto end = clock();
	context.time_cost = 1.0 * (end - start) / CLOCKS_PER_SEC;

	std::cout << "In Sample Size = " << nsamp << " Circuit Cost" << endl;
	PrintInfo(context);
	std::cout << endl;
}

void Q9Demo(CircSMPContext context) {
    vector<vector<uint32_t>> tuples;
    
	string filename = "../data/q9" + to_string(context.role) + ".shrdat";
    LoadData(filename, tuples);

	uint32_t neles = tuples.size(), nattrs = tuples[0].size();
	vector<uint32_t> nsamps = {50, 100, 500, 1000, 5000, 10000, 50000, neles};
	// vector<uint32_t> nsamps = {neles};

	vector<uint64_t> keys(neles);
	for (auto i=0u; i<neles; ++i) {
		keys[i] = (context.role == CLIENT) ? 0 : neles - i;
	}
	std::cout << "Sort " << keys.size() << ',' << tuples.size() << " elements" << endl; 
	context.time_cost = 0;
	context.comm_cost = 0;
	ShuffleQSort(keys, tuples, context);
	PrintInfo(context);
	context.time_cost = 0;
	context.comm_cost = 0;
	gParty.Reset();
	
	for (auto nsamp : nsamps) {
		GenQ9Circ(tuples, nsamp, context);
	}
}

int main(int argc, char ** argv) {
    auto context = read_options(argc, argv);
    // Q1Demo(context);
	// Q8Demo(context);
	Q9Demo(context);
    return 0;
}
