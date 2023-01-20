'''
Q6 query without any restriction

SELECT
     SUM(l_extendedprice * l_discount) AS revenue
FROM
     lineitem

+-----------------------------------+----------+
| sum(l_extendedprice * l_discount) | count(*) |
+-----------------------------------+----------+
|                    107054818.3761 |    60175 |
+-----------------------------------+----------+

Global sensitivity, GS = 10^4
1. max(l_extendedprice) = 94949.50, max(l_discount) = 0.1
2. max(l_extendedprice * l_discount) = 9474.95

'''

import numpy as np
import math

def LapNoise():
    a = np.random.uniform(0, 1)
    b = math.log( 1 / (1-a) )
    c = np.random.uniform(0, 1)
    if c > 0.5:
        return b
    else:
        return -b

def load_data():
    data = []
    with open('q6.txt', 'r') as f:
        for line in f.readlines():
            data.append(float(line))
    return data

def lap_noise(value, epsilon):
    dpres = np.array([value + LapNoise() * GS / epsilon for i in range(repeat_times)])
    error = np.abs(dpres - value)
    return error

def Poisson_sample():
    epsilon_ = math.log(1 + (math.exp(epsilon) - 1) / sample_rate)
    # print (epsilon_)
    error = []
    error_sample = []
    error_dp = []
    for i in range(repeat_times):
        selbits = np.random.binomial(1, sample_rate, len(data))
        sum = 0
        for j in range(len(data)):
            if selbits[j] == 1:
                sum += data[j]
        sum = sum / sample_rate
        sample_error = abs(sum - real_result)
        dp_noise = LapNoise() * GS / (epsilon_)
        dp_error = abs(dp_noise)
        sum += dp_noise
        error.append(abs(sum - real_result))
        error_sample.append(sample_error)
        error_dp.append(dp_error)
    print ("POISSON,"+str(np.mean(error_sample))+ "," + str(np.mean(error_dp)))
    total[-1].append([np.mean(error_sample), np.mean(error_dp)])
    return np.array(error)

def Shuffle_sample():
    error = []
    error_sample = []
    error_dp = []
    for i in range(repeat_times):
        shuffle_data = data.copy()
        np.random.shuffle(shuffle_data)
        sum = 0
        for j in range(sample_size):
            sum += shuffle_data[j]
        sum = sum / sample_rate
        # print ("shuffle estimate", sum, real_result)
        sample_error = abs(sum - real_result)
        dp_noise = LapNoise() * GS / (epsilon)
        dp_error = abs(dp_noise)
        sum += dp_noise
        error.append(abs(sum - real_result))
        error_sample.append(sample_error)
        error_dp.append(dp_error)
    print ("SHUFFLE,"+str(np.mean(error_sample))+ "," + str(np.mean(error_dp)))
    total[-1].append([np.mean(error_sample), np.mean(error_dp)])
    return np.array(error)

def WoR_sample():
    epsilon_ = math.log(1 + (math.exp(epsilon) - 1) / sample_size * len(data))
    error = []
    error_sample = []
    error_dp = []
    for i in range(repeat_times):
        wor_data = np.random.choice(data, sample_size, replace = False)
        sum = np.sum(wor_data)
        sum = sum / sample_rate
        sample_error = abs(sum - real_result)
        dp_noise = LapNoise() * GS / (epsilon_)
        dp_error = abs(dp_noise)
        sum += dp_noise
        error.append(abs(sum - real_result))
        error_sample.append(sample_error)
        error_dp.append(dp_error)
    print ("WoR,"+str(np.mean(error_sample))+ "," + str(np.mean(error_dp)))
    total[-1].append([np.mean(error_sample), np.mean(error_dp)])
    return np.array(error)

def WR_sample():
    rho = 1 - ((1 - 1/len(data)) ** sample_size)
    epsilon_ = math.log(1 + (math.exp(epsilon) - 1) / rho)
    error = []
    error_sample = []
    error_dp = []
    for i in range(repeat_times):
        wr_data = np.random.choice(data, sample_size, replace = True)
        sum = np.sum(wr_data)
        sum = sum / sample_rate
        sample_error = abs(sum - real_result)
        dp_noise = LapNoise() * GS / (epsilon_)
        dp_error = abs(dp_noise)
        sum += dp_noise
        error.append(abs(sum - real_result))
        error_sample.append(sample_error)
        error_dp.append(dp_error)
    print ("WoR,"+str(np.mean(error_sample))+ "," + str(np.mean(error_dp)))
    total[-1].append([np.mean(error_sample), np.mean(error_dp)])
    return np.array(error)

if __name__ == "__main__":
    data = load_data()

    GS = 10000
    epsilon = 1e-3
    repeat_times = 100

    sample_rates = [0.001, 0.005, 0.01, 0.05, 0.1]
    total = []
    # sample_rates = [0.01]

    for sample_rate in sample_rates:
        print (sample_rate)
        total.append([])

        real_result = sum(data)
        error0 = lap_noise(real_result, epsilon)
        total[-1].append([0, np.mean(error0)])
        print ("EXACT,0," + str(np.mean(error0)))

        sample_size = int(sample_rate * len(data))
        error2 = Shuffle_sample()
        print ("   Error of Shuffle sampling,", np.mean(error2))

        error1 = Poisson_sample()
        print ("   Error of Poisson sampling,", np.mean(error1))

        error3 = WoR_sample()
        print ("   Error of WoR sampling,", np.mean(error3))

        error4 = WR_sample()
        print ("   Error of WR sampling,", np.mean(error4))


        print ()
    
    print (total)