#include <Windows.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <time.h>
#include "types.h"
#include "timer.h"

struct Task
{
	float		l;
	UserStats*	u;
	DataEntry*	data;
	float *		temp;
	Model *		mod_kNN;
	unsigned int* sp_R;
	unsigned int* sp_N;
	unsigned int* spMat;
	int			first;
	int			last;
};

float predict(	Model * kNN, 
								UserStats *u, 
								unsigned int * sp_R, 
								unsigned int * sp_N, 
								unsigned int * startpMatArray, 
								unsigned int user_u, 
								unsigned int movie_i, 
								DataEntry *trainset)
{
	float s								= 0;
	unsigned int _startindex_R			= sp_R[user_u];			// Place where ratings of user a in um/all.dta.train start
	unsigned int _startindex_N			= sp_N[user_u];			// Place where ratings of user a in um/all.dta.all start
	unsigned int _startindexMatArray	= startpMatArray[movie_i];	// Place where w(i, j) start in weightsarray for movie j
	unsigned int movie_j;
	float corrfactor;

	// Add biases
	s += MEAN_RATING + kNN->v_bu[user_u] + kNN->v_bm[movie_i];
	
	// Corr matrix is stored in order {21 -- N1, 32 -- N2, 43 -- N3, ... N N-1}
	
	// Add correlation with other movies with known ratings by user a
	for (int i = _startindex_R; i < _startindex_R + u->allRatedMoviesWithRatingByUserSize_R[user_u]; i++)
	{	// sum_j in R[a] (r_aj - B_aj ) * w_bj

		movie_j = u->allRatedMoviesWithRatingByUser_R[i];

		// Only movies not equal to b contribute to the sum
		if (trainset[i].movie_ptr == movie_i){ corrfactor = 0; }
		else{ corrfactor = (*kNN).m_corr_W[_startindexMatArray + movie_j - movie_i - 1]; };
		
		// (note: these are movies in category 1 or 3, and are filtered out by fillWithData into train_dataA)
		s += (trainset[i].rating - MEAN_RATING - (*kNN).v_bu[user_u] - (*kNN).v_bm[trainset[i].movie_ptr]) * corrfactor;
	};

	// Add baseline using all rated movies by user a
	for (int i = _startindex_N; i < _startindex_N + u->allRatedMoviesByUserSize_N[user_u]; i++)
	{
		movie_j = u->allRatedMoviesByUser_N[i];
		if (u->allRatedMoviesByUser_N[i] != movie_i){
			s += (*kNN).m_baseline_C[_startindexMatArray + movie_j - movie_i - 1];
		}
	}

	return s;
}

void init_mod_kNN(Model* mod_kNN)
{
	srand(time(NULL));
	for (int i = 0; i < USER_NUM; i++)
	{
		mod_kNN->v_bu[i] = SEEDRANGE_BIAS*(2 * float(rand()) / RAND_MAX - 1);
	}
	for (int i = 0; i < MOVIE_NUM; i++)
	{
		mod_kNN->v_bm[i] = SEEDRANGE_BIAS*(2 * float(rand()) / RAND_MAX - 1);
	}

	for (int i = 0; i < NUMBER_OF_UNIQUE_WEIGHTS; i++)
	{
		mod_kNN->m_corr_W[i]		= SEEDRANGE_CORR*(2 * float(rand()) / RAND_MAX - 1);
		mod_kNN->m_baseline_C[i]	= SEEDRANGE_BASELINE*(2 * float(rand()) / RAND_MAX - 1);
	}
}

float computeRMSE(	Model * kNN, 
					UserStats *u, 
					unsigned int * sp_R, 
					unsigned int * sp_N,
					unsigned int * spMatArray, 
					DataEntry* trainset, 
					int trainsize, 
					DataEntry* testset, 
					int testsize)
{
	double sum2 = 0;
	double p;
	for (int i = 0; i < testsize; i++)
	{
		p = predict(kNN, u, sp_R, sp_N, spMatArray, testset[i].user_ptr, testset[i].movie_ptr, trainset);
		sum2 += (p - testset[i].rating)*(p - testset[i].rating);
	}	
	return sqrt(sum2 / testsize);
}
	


DWORD WINAPI weightsThread(LPVOID param)
{	
	float biasu = 0.1;
	float biasm = 0.1;
	int userID_u;
	int pos, posMat;
	int movieID_i;
	int movieID_j;


	Task p = *(Task*)param;
	float * mm;
	
	for (int dp = p.first; dp <= p.last; dp++)
	{
		

		// For Data-point j, we have some movieID, and we have to loop through all movies in R[userID]
		userID_u	= p.data[dp].user_ptr;
		movieID_i	= p.data[dp].movie_ptr;

		// Update the movie-movie correlation matrix W
		unsigned int sizeR = p.u->allRatedMoviesWithRatingByUserSize_R[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_W for a movieID x
			// Use UserStats to find all movies that user i rated

			// First we find the k'th movie that userID_u rated 
			pos			= p.sp_R[movieID_i] + k;
			movieID_j	= p.u->allRatedMoviesWithRatingByUser_R[ pos ];
			
			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;
			
			// Find the biases of user u and movie j
			biasu = p.mod_kNN->v_bu[userID_u];
			biasm = p.mod_kNN->v_bm[movieID_j];

			mm = &p.mod_kNN->m_corr_W[posMat];

			*mm += p.l * (p.u->invSqRootSizeR[userID_u] * p.temp[dp] * (p.data[pos].rating - MEAN_RATING - biasu - biasm) 
														- REGULARIZATION_CONST_W * (*mm));
			// Note p.data[pos].rating is exactly the rating r_uk, because the movies in R appear 
			// in the same order in the data-set (which is user-movie ordered)

		};

		// Update the movie-movie baseline matrix C
		unsigned int sizeN = p.u->allRatedMoviesByUserSize_N[userID_u];

		for (int k = 0; k < sizeN; k++){

			// spMat is the startingpoint of entries in m_corr_C for a movieID x
			// Use UserStats to find all movies that user i rated (including with unknown ratings)

			// First we find the k'th movie that userID_u rated (including with unknown ratings)
			pos = p.sp_N[movieID_i] + k;
			movieID_j = p.u->allRatedMoviesByUser_N[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;
			
			mm = &p.mod_kNN->m_baseline_C[posMat];

			*mm += p.l * (p.u->invSqRootSizeN[userID_u] * p.temp[dp] 
												- REGULARIZATION_CONST_C * (*mm));

		};

		//// Update biases
		mm = &p.mod_kNN->v_bu[p.data[dp].user_ptr];
		*mm	+= B_LEARNING_RATIO *p.temp[dp];
		mm = &p.mod_kNN->v_bm[p.data[dp].movie_ptr];
		*mm	+= B_LEARNING_RATIO *p.temp[dp];

		/*for (int i = 0; i < SVD_dim; i++)
		{
			u = &p.data->userSVD[ p.data[j].user_ptr ].params[i];
			m = &p.mod_kNN->movieSVD[p.data[j].movie_ptr].params[i];
			h = *u;

			*u += t*(*m) - REGULARIZATION_CONST_U*p.l*(*u);
			*m += t*h - REGULARIZATION_CONST_M*p.l*(*m);
		}*/

	}

	return 0;
}



DWORD WINAPI errorThread(LPVOID param)
{
	Task t = *(Task*)param;
	DataEntry *tt;
	for (int i = t.first; i <= t.last; i++)
	{
		tt = &t.data[i];
		t.temp[i] = (tt->rating - predict(	t.mod_kNN,
											t.u,
											t.sp_R,
											t.sp_N,
											t.spMat,
											tt->user_ptr,
											tt->movie_ptr,
											t.data)
											);
	};
	return 0;
}


void trainParallelIteration(	Model* kNN, 
								UserStats* u,
								unsigned int * sp_R,
								unsigned int * sp_N,
								unsigned int * spMatArray,
								float l, 
								DataEntry* trainset, 
								float* temporary, 
								int trainsize)
{
	float		t, h = 0;
	DataEntry	*tt;
	timerStart(1);
	DWORD		tid[NUM_THREADS];
	Task		tsk[NUM_THREADS];
	HANDLE		thread[NUM_THREADS];

	////////////////////
	// Compute the error
	////////////////////l
	// Chop up tasks into parts for threads

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		tsk[i].first	= (trainsize / NUM_THREADS)*i;
		tsk[i].last		= (trainsize / NUM_THREADS)*(i + 1) - 1;
		tsk[i].data		= trainset;
		tsk[i].temp		= temporary;
		tsk[i].sp_R		= sp_R;
		tsk[i].sp_N		= sp_N;
		tsk[i].spMat	= spMatArray;
		tsk[i].l		= l;
		tsk[i].u		= u;
		tsk[i].mod_kNN	= kNN;

		thread[i] = CreateThread(NULL, 0, errorThread, &tsk[i], 0, &tid[i]);
	}

	// Take care of the remaining parts of the task
	for (int i = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); i < trainsize; i++)
	{
		tt = &trainset[i];
		temporary[i] = (tt->rating - predict(	kNN,
												u,
												sp_R,
												sp_N,
												spMatArray,
												tt->user_ptr,
												tt->movie_ptr,
												trainset));
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads 
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}
	printf("---errors:%.3f\n", timerGetS(1));





	// Train the weights
	timerStart(1);

	// All threads but the last
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		thread[i] = CreateThread(NULL, 0, weightsThread, &tsk[i], 0, &tid[i]);
	}

	// Last part
	
	float * m;
	float biasu;
	float biasm;
	int userID_u;
	int pos, posMat;
	int movieID_i;
	int movieID_j;

	for (int dp = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); dp < trainsize; dp++)
	{
		t = l*temporary[dp];
		// For Data-point j, we have some movieID, and we have to loop through all movies in R[userID]
		userID_u = trainset[dp].user_ptr;
		movieID_i = trainset[dp].movie_ptr;

		// Update the movie-movie correlation matrix W
		unsigned int sizeR = u->allRatedMoviesWithRatingByUserSize_R[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_W for a movieID x
			// Use UserStats to find all movies that user i rated

			// First we find the k'th movie that userID_u rated 
			pos = sp_R[movieID_i] + k;
			movieID_j = u->allRatedMoviesWithRatingByUser_R[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
			
			// Find the biases of user u and movie j
			biasu = kNN->v_bu[userID_u];
			biasm = kNN->v_bm[movieID_j];
			m = &kNN->m_corr_W[posMat];

			*m += l * (u->invSqRootSizeR[userID_u] * temporary[dp] * (trainset[pos].rating - MEAN_RATING - biasu - biasm) 
										- REGULARIZATION_CONST_W * (*m) );
			// Note trainset[pos].rating is exactly the rating r_uk, because the movies in R appear 
			// in the same order in the data-set (which is user-movie ordered)

		};

		// Update the movie-movie baseline matrix C
		unsigned int sizeN = u->allRatedMoviesByUserSize_N[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_C for a movieID x
			// Use UserStats to find all movies that user i rated (including with unknown ratings)

			// First we find the k'th movie that userID_u rated (including with unknown ratings)
			pos = sp_N[movieID_i] + k;
			movieID_j = u->allRatedMoviesByUser_N[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
			m = &kNN->m_baseline_C[posMat];

			*m += l * (u->invSqRootSizeN[userID_u] * temporary[dp] 
											- REGULARIZATION_CONST_C * (*m));

		};

		// Update biases
		m = &kNN->v_bu[trainset[dp].user_ptr];
		*m += B_LEARNING_RATIO  *temporary[dp];
		m = &kNN->v_bm[trainset[dp].movie_ptr];
		*m += B_LEARNING_RATIO *temporary[dp];
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}




	printf("---features:%.3f\n", timerGetS(1));

}



