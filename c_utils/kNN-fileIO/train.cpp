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
	unsigned int* sp;
	unsigned int* sp_N;
	unsigned int* spMat;
	int			first;
	int			last;
};

static inline float predict(	Model * kNN, 
								UserStats *u, 
								unsigned int * startp, 
								unsigned int * startpMatArray, 
								unsigned int a, 
								unsigned int b, 
								DataEntry *trainset)
{
	float s								= 0;
	unsigned int _startindex			= startp[a];			// Place where ratings of user a in um/all.dta.train start
	unsigned int _startindexMatArray	= startpMatArray[b];	// Place where w(i, j) start in weightsarray for movie j
	float corrfactor;

	// Add biases
	s += MEAN_RATING + kNN->v_bu[a] + kNN->v_bm[b];
	
	// Corr matrix is stored in order {21 -- N1, 32 -- N2, 43 -- N3, ... N N-1}
	// CHECK ORDER OF DATA IN UM_RATINGS: USER-MOVIE, M_COOR_W HAS BE ORDERED IN THE SAME WAY!!!!!
	// Add correlation with other movies with known ratings by user a
	for (int i = _startindex; i < _startindex + u->allRatedMoviesWithRatingByUserSize_R[a]; i++)
	{	// sum_j in R[a] (r_aj - B_aj ) * w_bj

		// Only movies not equal to b contribute to the sum
		if (trainset[i].movie_ptr == b){corrfactor = 0;}
		else{ corrfactor = (*kNN).m_corr_W[_startindexMatArray + trainset[i].movie_ptr - b - 1]; };

		s += (trainset[i].rating - MEAN_RATING - (*kNN).v_bu[a] - (*kNN).v_bm[trainset[i].movie_ptr]) * corrfactor;
	};

	// Add baseline using all rated movies by user a
	for (int i = _startindex; i < _startindex + u->allRatedMoviesByUserSize_N[a]; i++)
	{
		if (trainset[i].movie_ptr != b)	s += (*kNN).m_baseline_C[_startindexMatArray + trainset[i].movie_ptr - b - 1];
	};

	return s;
}

void init_mod_kNN(Model* mod_kNN)
{
	srand(time(NULL));
	for (int i = 0; i < USER_NUM; i++)
	{
		mod_kNN->v_bu[i] = SEEDRANGE*float(rand()) / RAND_MAX;
	}
	for (int i = 0; i < MOVIE_NUM; i++)
	{
		mod_kNN->v_bm[i] = SEEDRANGE*float(rand()) / RAND_MAX;
	}

	for (int i = 0; i < NUMBER_OF_UNIQUE_WEIGHTS; i++)
	{
		mod_kNN->m_corr_W[i]		= SEEDRANGE*float(rand()) / RAND_MAX;
		mod_kNN->m_baseline_C[i]	= SEEDRANGE*float(rand()) / RAND_MAX;
	}
}

float computeRMSE(	Model * kNN, 
					UserStats *u, 
					unsigned int * sp, 
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
		p = predict(kNN, u, sp, spMatArray, testset[i].user_ptr, testset[i].movie_ptr, trainset);
		sum2 += (p - testset[i].rating)*(p - testset[i].rating);
	}	
	return sqrt(sum2 / testsize);
}
	


DWORD WINAPI weightsThread(LPVOID param)
{
	float *weight;
	float *baseline;
	float biasu;
	float biasm;
	int userID_u;
	int pos, posMat;
	int movieID_i;
	int movieID_j;

	Task p = *(Task*)param;

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
			pos			= p.sp[movieID_i] + k;
			movieID_j	= p.u->allRatedMoviesWithRatingByUser_R[ pos ];
			
			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;
			weight = &p.mod_kNN->m_corr_W[posMat];

			// Find the biases of user u and movie j
			biasu = p.mod_kNN->v_bu[userID_u];
			biasm = p.mod_kNN->v_bm[movieID_j];

			*weight += p.l * (p.u->invSqRootSizeR[userID_u] * p.temp[dp] * (p.data[ pos ].rating - MEAN_RATING - biasu - biasm) - REGULARIZATION_CONST_W * (*weight));
			// Note p.data[pos].rating is exactly the rating r_uk, because the movies in R appear 
			// in the same order in the data-set (which is user-movie ordered)

		};

		// Update the movie-movie baseline matrix C
		unsigned int sizeN = p.u->allRatedMoviesByUserSize_N[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_C for a movieID x
			// Use UserStats to find all movies that user i rated (including with unknown ratings)

			// First we find the k'th movie that userID_u rated (including with unknown ratings)
			pos = p.sp_N[movieID_i] + k;
			movieID_j = p.u->allRatedMoviesByUser_N[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;
			baseline = &p.mod_kNN->m_baseline_C[posMat];

			*baseline += p.l * (p.u->invSqRootSizeN[userID_u] * p.temp[dp] - REGULARIZATION_CONST_C * (*baseline));

		};

		// Update biases
		p.mod_kNN->v_bu[p.data[dp].user_ptr] += B_LEARNING_RATIO  *p.temp[dp];
		p.mod_kNN->v_bm[p.data[dp].movie_ptr] += B_LEARNING_RATIO *p.temp[dp];

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
											t.sp,
											t.spMat,
											tt->user_ptr,
											tt->movie_ptr,
											t.data)
											);
			// (t.svd, tt->user_ptr, tt->movie_ptr));
	};
	return 0;
}


void trainParallelIteration(	Model* kNN, 
								UserStats* u,
								unsigned int * sp,
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
		tsk[i].sp		= sp;
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
												sp,
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

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		thread[i] = CreateThread(NULL, 0, weightsThread, &tsk[i], 0, &tid[i]);
	}

	// Chop up tasks into parts for threads
	/*for (int j = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); j < trainsize; j++)
	{
		t = l*temporary[j];
		for (int i = 0; i < SVD_dim; i++)
		{
			u = &kNN->userSVD[trainset[j].user_ptr].params[i];
			m = &kNN->movieSVD[trainset[j].movie_ptr].params[i];
			h = *u;
			*u += t*(*m) - REGULARIZATION_CONST_U*l*(*u);
			*m += t*h - REGULARIZATION_CONST_M*l*(*m);
		}
		svd->bu[trainset[j].user_ptr] += B_LEARNING_RATIO*t;
		svd->bm[trainset[j].movie_ptr] += B_LEARNING_RATIO*t;
	}*/

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}

	printf("---features:%.3f\n", timerGetS(1));

}



