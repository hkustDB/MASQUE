# MASQUE: Multi-party Approximate Secure QUEry processing system

We build MASQUE, an MPC-AQP system that evaluates approximate queries on pre-generated samples in the semi-honest two-server model. It is a demo system of paper "Secure Sampling for Approximate Multi-party Query Processing".

## System overview

The system involves several parties,

- any number of data owners (providing data);
- two semi-honest non-colluding servers (performing MPC calculation);
- a client (inquiry an aggregation query).

MASQUE takes a two-stage approach: 

- In the **offline** stage, MASQUE denormalizes the data using existing JOIN protocols if necessary and obtain a flat table in secret-shared form; and then the most improtantly, prepares a batch of samples (uniform sampling without replacement and stratified sampling with specific group-by key $k$);
- In the **online** stage, MASQUE takes the query from the client and constructs a query evaluation circuit with a pre-generated sample, also injects DP noises on query result with a circuit if necessary, and then evaluates the circuit and sents the shares to the client for result reconstruction.

![image-20230120120234134](/Users/luoqiyao/Library/Application Support/typora-user-images/image-20230120120234134.png)



## Installation guideline

Our work is based on ABY framework. To install our system on Linux, you should clone this repo from Github, and then compile these codes through cmake. Some of the requirements of the system`cmake (version >= 3.13)`, `g++ (vection >= 8) `, `libboost-all-dev (version >= 1.69) `. You can install these requirements by using `sudo apt-get install xxx` in Linux.

```
git clone --recursive https://github.com/sigmod2024/MASQUE.git
mkdir build; cd build
cmake ..
make -j 4
```



## Offline stage

MASQUE implements the batch sample generation in offline stage. It contains the following sampling methods,

- Shuffle sampling(`ShuffleSamples()`)
- With replacement sampling(`UniformWRSamples()`)
- Without replacement sampling (`WoRSamples()`)
- Stratified sampling (It has the same generation function as WoR sampling)

Notice that MASQUE implements each component with a seperate function (i.e., `GenWoRIndices()` generates a batch of random numbers without replacement in range $n$; `GetSamples()` picks samples from the dataset with given index), so you can check each component's cost respectively.

```
[In 'build' directory]
./offline 0 // In server 0
./offline 1 // In server 1

./offline -a [IP address] -p [Port ID] -r [role "0/1"]
```

The last command is a full instructions for use. You can specify IP address and port specifically with `-a` and `-p` . `-r` is a identifier of the two servers. You can also simplify the command by the first two lines in one machine, where the IP address is `127.0.0.1` and port is `8888`.



## Online stage

MASQUE also evaluates the query evaluation circuit in online stage. There are 4 sample queries (Q1, Q1U, Q8, Q9), where details are shown in the paper. And the commands to run online query evaluation is,

```
[In 'build' directory]
./online 0 // In server 0
./online 1 // In server 1

./online -a [IP address] -p [Port ID] -r [role "0/1"]
```

