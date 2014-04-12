__author__ = 'StephanZheng'

import numpy as np

Nusers = 100 # 22338
Nmovies = 100 # 17770
_regularization = 0.5

users = np.arange(Nusers) + 1
movies = np.arange(Nmovies) + 1
ratings = np.random.randint(1, 5, size=(Nusers, Nmovies))

globalaverage = np.mean(ratings) # 3.5075

print ratings
print globalaverage

bu = np.ones(Nusers) / 10
bm = np.ones(Nmovies) / 10

def targetFunction():
    sum = 0
    for i in range(0, Nusers):
        for j in range(0, Nmovies):
            sum += pow( ratings[i, j] - globalaverage - bu[i] - bm[j], 2 ) + _regularization * pow(bm[j], 2)
        sum += _regularization * pow(bu[i], 2)
    return sum

def findMinimumSGD(threshold, steps, batchsize, learningrate, regularization):
    for i in range(0, steps):

        _temp = targetFunction()

        # print "User run", i + 1
        for _user in range(0, bu.size):

            batch = [np.random.randint(0, Nmovies) for p in range(0, batchsize)]
            print batch
            _sum = 0
            for k in batch:
                _sum += ratings[_user, k] - globalaverage - bu[_user] - bm[k]

            bu[_user] -= learningrate * (regularization * 2 * bu[_user] - 2 * _sum)
            # print _user, bu[_user]

        # print "Movie run", i + 1
        for _movie in range(0, bm.size):
            temp = bm[_movie]
            batch = [np.random.randint(0, Nusers) for p in range(0, batchsize)]
            _sum = 0
            for k in batch:
                _sum += ratings[k, _movie] - globalaverage - bu[k] - bm[_movie]

            bm[_movie] -= learningrate * (regularization * 2 * bm[_movie] - 2 * _sum)
            # print _movie, bm[_movie]

        print targetFunction()

        if abs(targetFunction() - _temp) < threshold:
            print "Under threshold"
            break
    print bu
    print bm
findMinimumSGD(0.01, 100, 3, 0.1, _regularization)

