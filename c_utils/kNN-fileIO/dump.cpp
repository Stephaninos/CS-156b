//
//// Take care of the remaining parts of the task
//for (int i = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); i < trainsize; i++)
//{
//	tt = &trainset[i];
//	temporary[i] = (tt->rating - predict(kNN,
//		u,
//		sp_R,
//		sp_N,
//		spMatArray,
//		tt->user_ptr,
//		tt->movie_ptr,
//		trainset));
//}

// Last part

//float * m;
//float biasu;
//float biasm;
//int userID_u;
//int pos, posMat;
//int movieID_i;
//int movieID_j;

//for (int dp = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); dp < trainsize; dp++)
//{
//	t = l*temporary[dp];
//	// For Data-point j, we have some movieID, and we have to loop through all movies in R[userID]
//	userID_u = trainset[dp].user_ptr;
//	movieID_i = trainset[dp].movie_ptr;

//	// Update the movie-movie correlation matrix W
//	unsigned int sizeR = u->nMovRated_R[userID_u];

//	for (int k = 0; k < sizeR; k++){

//		// spMat is the startingpoint of entries in m_corr_W for a movieID x
//		// Use UserStats to find all movies that user i rated

//		// First we find the k'th movie that userID_u rated 
//		pos = sp_R[movieID_i] + k;
//		movieID_j = u->idMovRated_R[pos];

//		// Then we find the position of w_ij in the long array m_ccorr_W[] 
//		// Remember the ordering 21--N1, 32--N2, 43--N3, etc
//		posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
//		
//		// Find the biases of user u and movie j
//		biasu = kNN->v_bu[userID_u];
//		biasm = kNN->v_bm[movieID_j];
//		m = &kNN->m_corr_W[posMat];
//		
//		
//		// printf("o: %.8f \n ", *m);
//		
//		*m += l * (u->invSqRootSizeR[userID_u] * temporary[dp] * (trainset[pos].rating - FIXED_BIAS_SUBTRACTION)//- biasu - biasm) 
//									- REGULARIZATION_CONST_W * (*m) );
//		// Note trainset[pos].rating is exactly the rating r_uk, because the movies in R appear 
//		// in the same order in the data-set (which is user-movie ordered)
//		//printf("n: %.8f \n ", *m);

//		// std::cin.ignore(1);

//	};

//	// Update the movie-movie baseline matrix C
//	unsigned int sizeN = u->nMovSeen_N[userID_u];

//	for (int k = 0; k < sizeR; k++){

//		// spMat is the startingpoint of entries in m_corr_C for a movieID x
//		// Use UserStats to find all movies that user i rated (including with unknown ratings)

//		// First we find the k'th movie that userID_u rated (including with unknown ratings)
//		pos = sp_N[movieID_i] + k;
//		movieID_j = u->idMovSeen_N[pos];

//		// Then we find the position of w_ij in the long array m_ccorr_W[] 
//		// Remember the ordering 21--N1, 32--N2, 43--N3, etc
//		posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
//		m = &kNN->m_baseline_C[posMat];

//		*m += l * (u->invSqRootSizeN[userID_u] * temporary[dp] 
//										- REGULARIZATION_CONST_C * (*m));

//	};

//	// Update biases
//	m = &kNN->v_bu[trainset[dp].user_ptr];
//	*m += l *(temporary[dp] - REGULARIZATION_CONST_B * (*m));
//	m = &kNN->v_bm[trainset[dp].movie_ptr];
//	*m += l *(temporary[dp] - REGULARIZATION_CONST_B * (*m));
//}