__author__ = 'StephanZheng'

#pickle to save memory instance?

import pandas as pd
import numpy as np
import cProfile
import struct
import scipy.sparse as spysp
import itertools
import time
import datetime
from array import array

################################################################################################################
################################################################################################################
################################################################################################################
################################################################################################################
#   Data defs + I/O
################################################################################################################
################################################################################################################
################################################################################################################
################################################################################################################

# n_entries = 2749898
# filepath = "C:\\Users\\StephanZheng\\Dropbox\\Academic\\Courses\\Caltech courses\\CS156b Machine Learning\\data\\mu\\all.dta.test"

# raw data location
# =================
path = "C:\\Users\\StephanZheng\\Dropbox\\Academic\\Courses\\Caltech courses\\CS156b Machine Learning\\data\\mu\\result\\"
filepath = "C:\\Users\\StephanZheng\\Dropbox\\Academic\\Courses\\Caltech courses\\CS156b Machine Learning\\data\\mu\\all.dta.train"

# parameters
# ==========
# processed files
# all.dta
# all.idx
# found 102416306 entries, out of which
# train 1 : 94362233
# train 2 : 1965045
# train 3 : 1964391
# train 6 : 0
# probe   : 1374739
# test    : 2749898

n_entries   = 5000 # 94362233 + 1965045 + 1964391
n_factors   = 10
n_movies    = 17770
n_users     = 458293

learnrate_gamma = 0.005
reg_lambda      = 0.001

THRESHOLD       = 0.0001
MAXITERATIONS   = 30
INITIALBIAS_UI  = 3
seed_range      = 10 # pu, qi in [0, 0.1] initially


# data-placeholders
# =================
r_data          = np.zeros((n_entries, 5),              dtype=np.uint32)

################################################################################################################
################################################################################################################
###     global functions
################################################################################################################
################################################################################################################

##########!!!!  speed up: only 1 read and then process object in memory

def read_file(filepath):
    i = 0
    with open(filepath, "rb") as f:
        while True:
            r_data[i,0] = struct.unpack('i', f.read(4))[0]
            r_data[i,1] = struct.unpack('h', f.read(2))[0]
            r_data[i,2] = struct.unpack('h', f.read(2))[0]
            r_data[i,3] = struct.unpack('B', f.read(1))[0]
            r_data[i,4] = struct.unpack('B', f.read(1))[0]

            i += 1
            if i>=n_entries:
                break
    return

def computeRMSE(r_data, sp_data, sp_predict):
    x = 0
    for i in range(0, n_entries):
        x += pow(sp_data[r_data[i, 0]-1, r_data[i, 1]-1] - sp_predict[r_data[i, 0]-1, r_data[i, 1]-1], 2)
    return pow( x / n_entries, 0.5)

def writeResultsToFile(tottime, _newRMSE, mu, bias_user, bias_movie, lf_user_p, lf_movie_q):
    # write prediction for qual set to file
    ts = time.time()
    st = datetime.datetime.fromtimestamp(ts).strftime('%m%d-%H%M')

    _newRMSE    = int(np.floor(_newRMSE * 10000))   # in bips
    mu          = int(np.floor(mu * 10000))         # in bips

    out_bias_user   = path + st + "_" + str(tottime) + "_" + str(_newRMSE) + "_" + str(mu) + "out_bias_user.bin"
    out_bias_movie  = path + st + "_" + str(tottime) + "_" + str(_newRMSE) + "out_bias_movie.bin"
    out_lf_user_p   = path + st + "_" + str(tottime) + "_" + str(_newRMSE) + "out_lf_user_p.bin"
    out_lf_movie_q  = path + st + "_" + str(tottime) + "_" + str(_newRMSE) + "out_lf_movie_q.bin"

    bias_user.tofile(out_bias_user)
    bias_movie.tofile(out_bias_movie)
    lf_user_p.tofile(out_lf_user_p)
    lf_movie_q.tofile((out_lf_movie_q))

    return

def computeMatrixNorm(mat, n_rows, n_columns):

    x = 0
    for i in range(0, n_rows):
        for j in range(0, n_columns):
            x += mat[i, j] ** 2

    return x

def testOnValidationSet(biasmovieuser, lf_user_p, lf_movie_q):

    # read in test set user-movie pairs


    # compute ratings for user-movie pairs in validation / test set


    # write predicted ratings to file, format as in example.dta

    return



################################################################################################################
################################################################################################################
################################################################################################################
################################################################################################################
###     SVD++ functions
################################################################################################################
################################################################################################################
################################################################################################################
################################################################################################################



#######!!!!!!! make this faster
def initUserP(r_data):

    temp_p = np.zeros((n_factors, n_users), dtype=np.float16)

    for i in range(0, n_users):
        for j in range(0, n_factors):
            temp_p[j, i] = 0.1 # np.random.rand()/seed_range

    return temp_p

def initMovieQ(r_data):

    temp_q = np.zeros((n_factors, n_movies), dtype=np.float16)

    for i in range(0, n_movies):
        for j in range(0, n_factors):
            temp_q[j, i] = 0.1 # np.random.rand()/seed_range

    return temp_q

def updateMovieQ(mat_p, mat_q, sp_error):

    # learnrate_gamma * ( k x U * U x I - k x I) dimensions are ok
    temp_q = mat_q + learnrate_gamma * ( mat_p * sp_error - reg_lambda * mat_q )

    return temp_q

def updateUserP(mat_p, mat_q, sp_error):

    # learnrate_gamma * ( k x U * U x I - k x I) dimensions are ok
    temp_p = mat_p + learnrate_gamma * ( mat_q * (sp_error.T) - reg_lambda * mat_p )

    return temp_p


def updateBiasUser(bias_user, sp_error):

    error_user = sp_error.sum(axis=1)
    print "Error_user: ", error_user.T.nonzero()
    return bias_user + learnrate_gamma * ( error_user - reg_lambda * bias_user )

def updateBiasMovie(bias_movie, sp_error):

    error_movie = sp_error.sum(axis=0)
    print "Error_movie: ", error_movie

    return bias_movie + learnrate_gamma * ( error_movie - reg_lambda * bias_movie )










def nupdateMovieQ(mat_p, mat_q, sp_error, i, j):

    return mat_q[:, j] + learnrate_gamma * ( mat_p[:, i] * sp_error[i, j] - reg_lambda * mat_q[:, j] )

def nupdateUserP(mat_p, mat_q, sp_error, i, j):

    return mat_p[:, i] + learnrate_gamma * ( mat_q[:, j] * sp_error[i, j] - reg_lambda * mat_p[:, i] )


def nupdateBiasUser(bias_user, sp_error, i, j):

    return bias_user[i, 0] + learnrate_gamma * ( sp_error[i, j] - reg_lambda * bias_user[i, 0] )

def nupdateBiasMovie(bias_movie, sp_error, i, j):

    return bias_movie[0, j] + learnrate_gamma * ( sp_error[i, j] - reg_lambda * bias_movie[0, j] )

def computePredictedRating(mu, bias_user, bias_movie, lf_user_p, lf_movie_q, sp_data):

    indices = sp_data.nonzero()
    rui     = spysp.lil_matrix((n_users, n_movies), dtype=float)
    pT      = lf_user_p.T

    for i, j in itertools.izip(indices[0], indices[1]):
        rui[i, j] = mu + bias_user[i, 0] + bias_movie[0, j] + np.dot( pT[i, :], lf_movie_q[:, j] )

    return spysp.lil_matrix.tocsr(rui)

def runSVD(r_data):

    start_time = time.time()

    # read data
    read_file(filepath)

    # save data into sparse matrix
    data    = spysp.coo_matrix((r_data[:, 3], (r_data[:,0]-1, r_data[:,1]-1)), shape=(n_users, n_movies))
    sp_data = spysp.coo_matrix.tocsr(data)

    print "read file and saved r_data in sp_data"

    # initialize

    mu              = r_data[:, 3].mean()
    bias_user       = np.ones((n_users, 1)) * np.random.rand() / seed_range
    bias_movie      = np.ones((1, n_movies)) * np.random.rand() / seed_range
    lf_user_p       = initUserP(r_data)
    lf_movie_q      = initMovieQ(r_data)
    print "initialized"

    # compute initial sp_predict
    sp_predict  = computePredictedRating(mu, bias_user, bias_movie, lf_user_p, lf_movie_q, sp_data)

    # compute r_error
    sp_error    = sp_data - sp_predict

    indices = sp_data.nonzero()

    # start the loop
    for i in range(1, MAXITERATIONS):

        # compute error of prediction vs data
        # _oldTotalError = computeTotalError(r_data, r_predict, bias_user_mov, lf_user_p, lf_movie_q)

        _oldRMSE = computeRMSE(r_data, sp_data, sp_predict)
        print 'Run ', i, ' | Old RMSE: ', _oldRMSE

        # update weights using gradient descent - loop only over ratings in r_data!

        #########!!!!! Check sequence of updates, have to go over data points one at a time, each time updating all the weights,
        #  instead of doing it like downstairs...
        # bias_user       = updateBiasUser(bias_user, sp_error)
        # bias_movie      = updateBiasMovie(bias_movie, sp_error)
        # lf_user_p       = updateUserP(lf_user_p, lf_movie_q, sp_error)
        # lf_movie_q      = updateMovieQ(lf_user_p, lf_movie_q, sp_error)

        # update weights by looping over all ratings in training-set

        for k, l in itertools.izip(indices[0], indices[1]):
            bias_user[k, 0]       = nupdateBiasUser(bias_user, sp_error, k, l)
            bias_movie[0, l]      = nupdateBiasMovie(bias_movie, sp_error, k, l)
            lf_user_p[:, k]       = nupdateUserP(lf_user_p, lf_movie_q, sp_error, k, l)
            lf_movie_q[:, l]      = nupdateMovieQ(lf_user_p, lf_movie_q, sp_error, k, l)

        print bias_user[0:10, 0]
        print bias_movie[0, 0:10]
        print

        # compute sp_predict
        sp_predict  = computePredictedRating(mu, bias_user, bias_movie, lf_user_p, lf_movie_q, sp_data)
        sp_error    = sp_data - sp_predict

        # compute error of prediction vs data AGAIN
        # _totalError = computeTotalError(r_data, r_predict, bias_user_mov, lf_user_p, lf_movie_q)
        _newRMSE    = computeRMSE(r_data, sp_data, sp_predict)

        # compare old vs new total error
        if abs(_oldRMSE - _newRMSE) < THRESHOLD:
            break

        print 'Run ', i, ' | new RMSE: ', _newRMSE

    # we converged or hit the max number of iterations
    print "Final RMSE: ", _newRMSE

    tottime = int(time.time() - start_time / 60)

    writeResultsToFile(tottime, _newRMSE, mu, bias_user, bias_movie, lf_user_p, lf_movie_q)

    return

################################################################################################################
################################################################################################################
###     Unit tests
################################################################################################################
################################################################################################################

def test1(r_data, r_predict, r_error, bias_user_mov, lf_user_p, lf_movie_q):
    return

################################################################################################################
################################################################################################################
###     Main routine
################################################################################################################
################################################################################################################

cProfile.run('runSVD(r_data)', sort='tottime')
# cProfile.run('test1(r_data, r_predict, r_error, bias_user_mov, lf_user_p, lf_movie_q)', sort='tottime')
# runSVD(r_data)


# To do:
# - fix bug in bias_movie, when the algo considers more than 1 movie
# - Test predictors on validation sets
# - Cap off values >5 and <1

