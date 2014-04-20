__author__ = 'StephanZheng'



# data = pd.io.parsers.read_csv(filepath, dtype=int, header=None, delim_whitespace=True)
# data.columns = ['userID', 'movieID', 'timestamp', 'rating']
# trainset = data[data['rating']!=0]

# # read entire file into memory. Then splice.
#
#
# def read_file(filepath):
#     i = 0
#     with open(filepath, "rb") as f:
#         while True:
#
#             data2[i,0] = struct.unpack('Q', f.read(8))[0]
#
#             i += 1
#             if i>=n_entries:
#                 break
#
#             # do_stuff_with(ord(byte))

#
# data2 = np.zeros(shape=(n_entries, 5), dtype=int)
#
# def read_file3(filepath):
#     file = open(filepath, "rb").read()
#     print file[0:10]
#     print type(file)
#     for i in range(0, n_entries):
#         data2[i,0]=   struct.unpack('i', file[i:i+4])[0]
#         data2[i,1]=   struct.unpack('h', file[i+4:i+6])[0]
#         data2[i,2]=   struct.unpack('h', file[i+6:i+8])[0]
#         data2[i,3]=   struct.unpack('B', file[i+8])[0]
#         data2[i,4]=   struct.unpack('B', file[i+9])[0]
#     print data2[1:20,:]





#
# print type(initializeSparseMatrix(r_data))
# print initializeSparseMatrix(r_data)
#
# lf_user_p = initializeSparseMatrix(r_data)
# x = initializeSparseMatrix(r_data)
# # print lf_user_p[:,1:20].todense()
# print x
#
#



def initializeSparseMatrices(r_data, bias_mov_user, _lf_user_p, _lf_movie_q):

    temp_p = spysp.lil_matrix((n_factors, n_users), dtype=float)
    temp_q = spysp.lil_matrix((n_factors, n_users), dtype=float)

    for i in range(0, n_entries):

        bias_mov_user[r_data[i,0], r_data[i,1]] = INITIALBIAS_UI

        # set as lil.matrix which is efficient in adding non-zero entries

        # if the user row in p is 0 (has no latent factor), and user appears in r_data, then randomize all latent factors for that user
        # if temp_p[0, r_data[i, 0]] == 0.:
        temp_p[:, r_data[i, 0]] = 0.1


        # if the movie row in q is 0 (has no latent factor), and movie appears in r_data, then randomize all latent factors for that movie
        # if temp_q[0, r_data[i, 1]] == 0.:
        temp_q[:, r_data[i, 1]] = 0.1 # np.random.rand()

    # translate lil matrix back to csr, which is efficient for computations

    _lf_user_p       = spysp.lil_matrix.tocsr(temp_p)
    _lf_movie_q      = spysp.lil_matrix.tocsr(temp_q)

    print _lf_user_p[:,1:20]

    return


# def initBiasMovieUser(r_data):
#
#     temp = spysp.lil_matrix((n_users, n_movies), dtype=float)
#
#     for i in range(0, n_entries):
#         temp[r_data[i, 0]-1, r_data[i, 1]-1] = INITIALBIAS_UI
#
#     return spysp.lil_matrix.tocsr(temp)
#




def updateWeights(row_id, r_data, r_error, bias_user_mov, lf_user_p, lf_movie_q):

    # print "r_error: ", r_error[1:10]

    # error                       = r_data[user, movie] - r_predict[user, movie]
    bias_user_mov[r_data[row_id, 0], r_data[row_id, 1]]  = bias_user_mov[r_data[row_id, 0], r_data[row_id, 1]]    + learnrate_gamma * ( r_error[row_id] - reg_lambda * bias_user_mov[r_data[row_id, 0], r_data[row_id, 1]])

    ##########!!!!  make this more efficient - use matrix library for this
    for factor in range(0, n_factors):
        lf_movie_q[factor, r_data[row_id, 1]]           = lf_movie_q[factor, r_data[row_id, 1]]             + learnrate_gamma * ( r_error[row_id] * lf_user_p[factor, r_data[row_id, 0]] - reg_lambda * lf_movie_q[factor, r_data[row_id, 1]])

    for factor in range(0, n_factors):
        lf_user_p[factor, r_data[row_id, 0]]             = lf_user_p[factor, r_data[row_id, 0]]               + learnrate_gamma * ( r_error[row_id] * lf_movie_q[factor, r_data[row_id, 1]] - reg_lambda * lf_user_p[factor, r_data[row_id, 0]])

    return


# r_predict       = np.zeros(n_entries,                   dtype=float)    # use same ordering as in r_data
# r_error         = np.zeros(n_entries,                   dtype=float)
# sp_data         = spysp.lil_matrix((n_users, n_movies), dtype=int)
# sp_predict      = spysp.lil_matrix((n_users, n_movies), dtype=float)
# sp_error        = spysp.lil_matrix((n_users, n_movies), dtype=float)

# Don't define globally, only locally within runSVD()
# bias_user_mov   = spysp.csr_matrix((n_users, n_movies), dtype=float)
# lf_user_p       = spysp.csr_matrix((n_factors, n_users),  dtype=float)
# lf_movie_q      = spysp.csr_matrix((n_factors, n_movies), dtype=float)


# lf_user_y       = spysp.csr_matrix((n_users, n_factors),  dtype=float)
# n_items_rated_N = np.zeros(shape=(n_users, 1),          dtype=int)







# # Don't use this, just use RMSE as gauge
# def computeTotalError(r_data, r_predict, bias_user_mov, lf_user_p, lf_movie_q):
#
#     x = 0
#
#     ##########!!!! need fast BLAS routine
#     for i in range(0, n_entries):
#         x += pow(r_data[i, 3] - r_predict[i], 2) + pow(bias_user_mov[r_data[i, 0]-1, r_data[i, 1]-1], 2)
#
#     # compute matrix norm of p, q: note that q * q.T and p * p.T have dimension n_factor x n_factor
#
#     temp = lf_movie_q.dot(lf_movie_q.T)
#     temp2 = lf_user_p.dot(lf_user_p.T)
#
#     for i in range(0, n_factors):
#         for j in range(0, n_factors):
#             x += temp[i, j] ** 2 + temp2[i, j] ** 2
#
#     return x







def computePQ(lf_user_p, lf_movie_q, sp_data):

    indices = sp_data.nonzero()
    pq      = spysp.lil_matrix((n_users, n_movies), dtype=float)

    pT      = lf_user_p.T

    for i, j in itertools.izip(indices[0], indices[1]):
        pq[i, j] = np.dot( pT[i, :], lf_movie_q[:, j] )

    return spysp.lil_matrix.tocsr(pq)



def updateBiasUI(bias_user_mov, sp_error):

    # learnrate_gamma * ( k x U * U x I - k x I) dimensions are ok
    temp_bias_user_mov = bias_user_mov + learnrate_gamma * (sp_error - reg_lambda * bias_user_mov)

    return temp_bias_user_mov
