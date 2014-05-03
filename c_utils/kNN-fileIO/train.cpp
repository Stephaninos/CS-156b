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
struct TaskRmse
{
	UserStats*		u;
	DataEntry*		trainset;
	DataEntry*		testset;
	Model*			mod_kNN;
	unsigned int*	sp_R;
	unsigned int*	sp_N;
	unsigned int*	spMat;
	int				first;
	int				last;
	float*			sum2;
};

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
		mod_kNN->m_corr_W[i] = SEEDRANGE_CORR*(2 * float(rand()) / RAND_MAX - 1);
		mod_kNN->m_baseline_C[i] = SEEDRANGE_BASELINE*(2 * float(rand()) / RAND_MAX - 1);
	}
}

// TODO make inline

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
	s += kNN->v_bu[user_u] + kNN->v_bm[movie_i]; // MEAN_RATING + 
	
	// Corr matrix is stored in order {21 -- N1, 32 -- N2, 43 -- N3, ... N N-1}	
	// Add correlation with other movies with known ratings by user user_u

	for (int i = _startindex_R; i < _startindex_R + u->nMovRated_R[user_u]; i++)
	{	// sum_j in R[a] (r_aj - B_aj ) * w_bj

		movie_j = u->idMovRated_R[i];

		// Only movies not equal to b contribute to the sum
		if (trainset[i].movie_ptr == movie_i){ corrfactor = 0; }
		else{ corrfactor = (*kNN).m_corr_W[_startindexMatArray + movie_j - movie_i - 1]; }
		
		// (note: these are movies in category 1 or 3, and are filtered out by fillWithData into train_dataA)
		s += (trainset[i].rating - (*kNN).v_bu[user_u] - (*kNN).v_bm[trainset[i].movie_ptr]) * corrfactor;
	};

	// Add baseline using all rated movies by user a
	for (int i = _startindex_N; i < _startindex_N + u->nMovSeen_N[user_u]; i++)
	{
		movie_j = u->idMovSeen_N[i];
		if (u->idMovSeen_N[i] != movie_i){
			s += (*kNN).m_baseline_C[_startindexMatArray + movie_j - movie_i - 1];
		}
	}

	return s;
}

float computeRMSE(	Model * kNN, 
					UserStats *u, 
					unsigned int * sp_R, 
					unsigned int * sp_N,
					unsigned int * spMatArray, 
					DataEntry* trainset, 
					DataEntry* testset, 
					int testsize)
{
	double sum2 = 0;
	double p;
	for (int i = 0; i < testsize; i++)
	{
		p = predict(kNN, u, sp_R, sp_N, spMatArray, testset[i].user_ptr, testset[i].movie_ptr, trainset);
		sum2 += (p - testset[i].rating)*(p - testset[i].rating);
		
		/*FILE * f = fopen("computeRMSE.csv", "at");
		float diff = testset[i].rating - p;
		if(testset[i].user_ptr==0) printf("%u, %u, %.3f, %.3f, %.3f \n", 
			testset[i].user_ptr, testset[i].movie_ptr, MEAN_RATING + testset[i].rating, MEAN_RATING + p, diff);
		
		fclose(f);*/

		//float diff = testset[i].rating - p;
		//if (testset[i].user_ptr == 0) printf("%u, %u, %.3f, %.3f, %.3f \n",
		//	testset[i].user_ptr, testset[i].movie_ptr, MEAN_RATING + testset[i].rating, MEAN_RATING + p, diff);

	}	
	return sqrt(sum2 / testsize);
}
	
DWORD WINAPI rmseThread(LPVOID param)
{
	// Do RMSE computation using thread input 'param', function returns a partial sum
	TaskRmse t = *(TaskRmse*)param;

	double p;
	float *x;
	x = t.sum2;

	for (int i = t.first; i <= t.last; i++)
	{
		p = predict(t.mod_kNN, t.u, t.sp_R, t.sp_N, t.spMat, t.testset[i].user_ptr, t.testset[i].movie_ptr, t.trainset);
		*x += (p - t.testset[i].rating)*(p - t.testset[i].rating);
	}

	// Need to update struct member by reference
	
	printf("Thread computed: %.3f\n", x);
	return 0;
}

float computeRmseParallel(
	Model* kNN,
	UserStats* u,
	unsigned int * sp_R,
	unsigned int * sp_N,
	unsigned int * spMatArray,
	DataEntry* trainset,
	DataEntry* testset,
	int testsize
	)
{
	DWORD		tid[NUM_THREADS];
	TaskRmse	tsk[NUM_THREADS];
	HANDLE		thread[NUM_THREADS];
	
	float result = 0;
	float * ssquares = new float;
	*ssquares = 0;

	// Chop up task in pieces and create threads
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		// Ingredients to compute rating - predicted value for i running over data-points
		tsk[i].u = u;
		tsk[i].trainset = trainset;
		tsk[i].testset = testset;
		tsk[i].mod_kNN = kNN;
		tsk[i].sp_R = sp_R;
		tsk[i].sp_N = sp_N;
//		tsk[i].sum2 = ssquares;
		tsk[i].spMat = spMatArray;
		tsk[i].first = (testsize / NUM_THREADS)*i;
		tsk[i].last = (testsize / NUM_THREADS)*(i + 1) - 1;
		thread[i] = CreateThread(NULL, 0, rmseThread, &tsk[i], 0, &tid[i]);
	}

	DataEntry*	tt;
	float		p = 0;

	// Take care of the remaining parts of the task
	for (int i = (testsize / NUM_THREADS)*(NUM_THREADS - 1); i < testsize; i++)
	{
		// Compute sum of squares of remaining predictions
		p = predict(kNN, u, sp_R, sp_N, spMatArray, testset[i].user_ptr, testset[i].movie_ptr, trainset);
		*ssquares += (p - testset[i].rating)*(p - testset[i].rating);
	}


	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);


	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		*ssquares += *(tsk[i].sum2);
		printf("sum2 %u: %.3f \n", i, ssquares);
	}

	// Close threads 
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}

	result = pow(*ssquares, 0.5);
	printf("%.3f\n", result);
	printf("%.3f\n", sqrt(*ssquares));
	delete ssquares;

	return result;

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

		// printf("Datapoint: %u, u %u, m %u \n", dp, userID_u, movieID_i);

		// Update the movie-movie correlation matrix W		
 		unsigned int sizeR = p.u->nMovRated_R[userID_u];

		if (sizeR != 0){
			for (int k = 0; k < sizeR; k++){

				// spMat is the startingpoint of entries in m_corr_W for a movieID x
				// Use UserStats to find all movies that user i rated

				// First we find the k'th movie that userID_u rated 
				pos = p.sp_R[userID_u] + k;
				movieID_j = p.u->idMovRated_R[pos];

				// Then we find the position of w_ij in the long array m_ccorr_W[] 
				// Remember the ordering 21--N1, 32--N2, 43--N3, etc
				posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;

				// Find the biases of user u and movie j
				biasu = p.mod_kNN->v_bu[userID_u];
				biasm = p.mod_kNN->v_bm[movieID_j];

				mm = &p.mod_kNN->m_corr_W[posMat];

				*mm += p.l * (p.u->invSqRootSizeR[userID_u] * p.temp[dp] * (p.data[pos].rating - FIXED_BIAS_SUBTRACTION) // - biasu - biasm) 
					- REGULARIZATION_CONST_W * (*mm));
				// Note p.data[pos].rating is exactly the rating r_uk, because the movies in R appear 
				// in the same order in the data-set (which is user-movie ordered)

			};
		}
		else{
			printf("SizeR = 0!");
		}

		// Update the movie-movie baseline matrix C
		unsigned int sizeN = p.u->nMovSeen_N[userID_u];

		if (sizeN != 0){

			for (int k = 0; k < sizeN; k++){

				// spMat is the startingpoint of entries in m_corr_C for a movieID x
				// Use UserStats to find all movies that user i rated (including with unknown ratings)

				// First we find the k'th movie that userID_u rated (including with unknown ratings)
				pos = p.sp_N[userID_u] + k;
				movieID_j = p.u->idMovSeen_N[pos];

				// Then we find the position of w_ij in the long array m_ccorr_W[] 
				// Remember the ordering 21--N1, 32--N2, 43--N3, etc
				posMat = p.spMat[movieID_i] + movieID_j - movieID_i - 1;

				mm = &p.mod_kNN->m_baseline_C[posMat];

				*mm += p.l * (p.u->invSqRootSizeN[userID_u] * p.temp[dp]
					- REGULARIZATION_CONST_C * (*mm));

			};

			//// Update biases
			mm = &p.mod_kNN->v_bu[p.data[dp].user_ptr];
			*mm += p.l * (p.temp[dp] - REGULARIZATION_CONST_B * (*mm));
			mm = &p.mod_kNN->v_bm[p.data[dp].movie_ptr]; // CHECK: WAS movie_ptr - 1
			*mm += p.l * (p.temp[dp] - REGULARIZATION_CONST_B * (*mm));

			/*for (int i = 0; i < SVD_dim; i++)
			{
			u = &p.data->userSVD[ p.data[j].user_ptr ].params[i];
			m = &p.mod_kNN->movieSVD[p.data[j].movie_ptr].params[i];
			h = *u;

			*u += t*(*m) - REGULARIZATION_CONST_U*p.l*(*u);
			*m += t*h - REGULARIZATION_CONST_M*p.l*(*m);
			}*/
		}
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
	
		if (i<1000 && t.temp[i] > THRESHOLD_ERROR_TEST){
			printf("Oh-oh, error for r[%u, %u] = %.4f\n", tt->user_ptr, tt->movie_ptr, t.temp[i]);
		}

	}

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
		if ((trainsize / NUM_THREADS)*(i + 1) - 1 > trainsize) tsk[i].last = trainsize - 1;

		tsk[i].first	= (trainsize / NUM_THREADS)*i;
		tsk[i].last		= (trainsize / NUM_THREADS)*(i + 1) - 1;
		tsk[i].data		= trainset;
		tsk[i].temp		= temporary;	// errors computed
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
	
	printf("---CPU-time errors   : %.3f\n", timerGetS(1));





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
		unsigned int sizeR = u->nMovRated_R[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_W for a movieID x
			// Use UserStats to find all movies that user i rated

			// First we find the k'th movie that userID_u rated 
			pos = sp_R[movieID_i] + k;
			movieID_j = u->idMovRated_R[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
			
			// Find the biases of user u and movie j
			biasu = kNN->v_bu[userID_u];
			biasm = kNN->v_bm[movieID_j];
			m = &kNN->m_corr_W[posMat];
			
			
			// printf("o: %.8f \n ", *m);
			
			*m += l * (u->invSqRootSizeR[userID_u] * temporary[dp] * (trainset[pos].rating - FIXED_BIAS_SUBTRACTION)//- biasu - biasm) 
										- REGULARIZATION_CONST_W * (*m) );
			// Note trainset[pos].rating is exactly the rating r_uk, because the movies in R appear 
			// in the same order in the data-set (which is user-movie ordered)
			//printf("n: %.8f \n ", *m);

			// std::cin.ignore(1);

		};

		// Update the movie-movie baseline matrix C
		unsigned int sizeN = u->nMovSeen_N[userID_u];

		for (int k = 0; k < sizeR; k++){

			// spMat is the startingpoint of entries in m_corr_C for a movieID x
			// Use UserStats to find all movies that user i rated (including with unknown ratings)

			// First we find the k'th movie that userID_u rated (including with unknown ratings)
			pos = sp_N[movieID_i] + k;
			movieID_j = u->idMovSeen_N[pos];

			// Then we find the position of w_ij in the long array m_ccorr_W[] 
			// Remember the ordering 21--N1, 32--N2, 43--N3, etc
			posMat = spMatArray[movieID_i] + movieID_j - movieID_i - 1;
			m = &kNN->m_baseline_C[posMat];

			*m += l * (u->invSqRootSizeN[userID_u] * temporary[dp] 
											- REGULARIZATION_CONST_C * (*m));

		};

		// Update biases
		m = &kNN->v_bu[trainset[dp].user_ptr];
		*m += l *(temporary[dp] - REGULARIZATION_CONST_B * (*m));
		m = &kNN->v_bm[trainset[dp].movie_ptr];
		*m += l *(temporary[dp] - REGULARIZATION_CONST_B * (*m));
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}




	printf("---CPU-time features : %.3f\n", timerGetS(1));
	
}



