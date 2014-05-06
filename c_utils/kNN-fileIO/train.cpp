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
	Biases *	biases;
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
	Biases*			biases;
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


inline float predict(			Model * kNN, 
								Biases * biases,
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
	// unsigned int _startindexMatArray	= startpMatArray[movie_i];	// Place where w(i, j) start in weightsarray for movie j
	unsigned int movie_j;
	unsigned int base = movie_i * MOVIE_NUM;
	float corrfactor;

	// Add biases
	s += kNN->v_bu[user_u] + kNN->v_bm[movie_i]; // MEAN_RATING + 

	// Add correlation with other movies with known ratings by user user_u
	for (int id = _startindex_R; id < _startindex_R + u->nMovRated_R[user_u]; id++)
	{	
		DataEntry cur = trainset[id];
		movie_j = cur.movie_ptr;									// u->idMovRated_R[i];
		if (movie_j == movie_i){ continue; }						// Only movies not equal to b contribute to the sum
		else{ corrfactor = (*kNN).m_corr_W[base + movie_j]; }

		s += u->invSqRootSizeR[user_u] * (cur.rating - (*biases).v_bu[user_u] - (*biases).v_bm[trainset[id].movie_ptr]) * corrfactor; // sum_j in R[a] (r_aj - B_aj ) * w_bj
		
		// (note: these are movies in category 1 or 3, and are filtered out by fillWithData into train_dataA)
	};

	// Add baseline using all rated movies by user a
	for (int id = _startindex_N; id < _startindex_N + u->nMovSeen_N[user_u]; id++)
	{
		movie_j = u->idMovSeen_N[id];
		if (movie_j == movie_i) continue;
		s += u->invSqRootSizeN[user_u] * (*kNN).m_baseline_C[base + movie_j];
	}

	return s;
}

float computeRMSE(	Model * kNN, 
					Biases * biases,
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
		p = predict(kNN, biases, u, sp_R, sp_N, spMatArray, testset[i].user_ptr, testset[i].movie_ptr, trainset);
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

	float p;
	float *x;
	x = t.sum2;

	for (int i = t.first; i <= t.last; i++)
	{
		p = predict(t.mod_kNN, t.biases, t.u, t.sp_R, t.sp_N, t.spMat, t.testset[i].user_ptr, t.testset[i].movie_ptr, t.trainset);
		*x += (p - t.testset[i].rating)*(p - t.testset[i].rating);
	}

	return 0;
}

float computeRmseParallel(
	Model* kNN,
	Biases * biases,
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
	
	float * ssquares = new float[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) ssquares[i] = 0.;

	// Chop up task in pieces and create threads
	for (int i = 0; i < NUM_THREADS; i++)
	{
		// Ingredients to compute rating - predicted value for i running over data-points
		tsk[i].u = u;
		tsk[i].trainset = trainset;
		tsk[i].testset = testset;
		tsk[i].mod_kNN = kNN;
		tsk[i].biases = biases;
		tsk[i].sp_R = sp_R;
		tsk[i].sp_N = sp_N;
		tsk[i].sum2 = &ssquares[i];
		tsk[i].spMat = spMatArray; 
		tsk[i].first = int(testsize / NUM_THREADS) * i; 

		if (i == NUM_THREADS - 1){ tsk[i].last = testsize - 1; }
		else { tsk[i].last = int(testsize / NUM_THREADS) * (i + 1) - 1; }

		// printf("Batch %u: %u, %u \n", i, tsk[i].first, tsk[i].last);

		thread[i] = CreateThread(NULL, 0, rmseThread, &tsk[i], 0, &tid[i]);
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads 
	for (int i = 0; i < NUM_THREADS; i++) CloseHandle(thread[i]);

	float ss;
	for (int i = 0; i < NUM_THREADS; i++)
	{
		ss += ssquares[i];
		// printf("sum2 %u: %.6f \n", i, ss);
	}

	delete ssquares;

	return pow(ss / testsize, 0.5);

}

DWORD WINAPI weightsThread(LPVOID param)
{	
	float prec_biasu = 0.1;
	float prec_biasm = 0.1;
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

		int base = movieID_i * MOVIE_NUM;

		// printf("Datapoint: %u, u %u, m %u \n", dp, userID_u, movieID_i);

		// Update the movie-movie correlation matrix W				
		for (int k = 0; k < p.u->nMovRated_R[userID_u]; k++){

				pos = p.sp_R[userID_u] + k;
				movieID_j = p.u->idMovRated_R[pos];
				
				// Find the biases of user u and movie j
				prec_biasu = p.biases->v_bu[userID_u];
				prec_biasm = p.biases->v_bm[movieID_j];

				mm = &p.mod_kNN->m_corr_W[ base + movieID_j ];

				*mm += p.l * (p.u->invSqRootSizeR[userID_u] * p.temp[dp] * (p.data[pos].rating - prec_biasu - prec_biasm) // - biasu - biasm) 
					- REGULARIZATION_CONST_W * (*mm));
				
		}

		// Update the movie-movie baseline matrix C		
		for (int k = 0; k < p.u->nMovSeen_N[userID_u]; k++){
			
			pos = p.sp_N[userID_u] + k;
			movieID_j = p.u->idMovSeen_N[pos];

			// movieID_j = p.u->idMovSeen_N[] // p.data[dp + k].movie_ptr;
				
			mm = &p.mod_kNN->m_baseline_C[base + movieID_j];

			*mm += p.l * (p.u->invSqRootSizeN[userID_u] * p.temp[dp]
				- REGULARIZATION_CONST_C * (*mm));

		};

		// Update biases
		mm = &p.mod_kNN->v_bu[p.data[dp].user_ptr];
		*mm += p.l * (p.temp[dp] - REGULARIZATION_CONST_B * (*mm));
		mm = &p.mod_kNN->v_bm[p.data[dp].movie_ptr]; // CHECK: WAS movie_ptr - 1
		*mm += p.l * (p.temp[dp] - REGULARIZATION_CONST_B * (*mm));
				
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
											t.biases,
											t.u,
											t.sp_R,
											t.sp_N,
											t.spMat,
											tt->user_ptr,
											tt->movie_ptr,
											t.data)
											);
	
		if (i<1000 && t.temp[i] > THRESHOLD_ERROR_TEST){
			printf("Oh-oh, error for r[%u, %u] = %.4f | Predicted: %.4f\n", tt->user_ptr, tt->movie_ptr, t.temp[i], tt->rating-t.temp[i] );
		}

	}

	return 0;
}



void trainParallelIteration(Model* kNN,
	Biases * biases,
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
	

	printf("User x: sp_R %u \n", sp_R[USER_NUM-1]);

	for (int i = 0; i < NUM_THREADS; i++)
	{
		tsk[i].first = sp_R[int(USER_NUM / NUM_THREADS) * i]; //(trainsize / NUM_THREADS)*i;

		if (i == NUM_THREADS - 1){
			tsk[i].last = trainsize - 1;
		}
		else
		{
			tsk[i].last = sp_R[int(USER_NUM / NUM_THREADS) * (i + 1)] - 1; // (trainsize / NUM_THREADS)*(i + 1) - 1;
		}

		// printf("Batch %u, f: %u, l: %u \n", i, tsk[i].first, tsk[i].last);

		tsk[i].data		= trainset;
		tsk[i].temp		= temporary;	// errors computed
		tsk[i].sp_R		= sp_R;
		tsk[i].sp_N		= sp_N;
		tsk[i].spMat	= spMatArray;
		tsk[i].l		= l;
		tsk[i].u		= u;
		tsk[i].mod_kNN	= kNN;
		tsk[i].biases = biases;

		thread[i] = CreateThread(NULL, 0, errorThread, &tsk[i], 0, &tid[i]);
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads 
	for (int i = 0; i < NUM_THREADS; i++)
	{
		CloseHandle(thread[i]);
	}
	
	printf("---CPU-time errors   : %.3f\n", timerGetS(1));
	
	// Train the weights
	timerStart(1);

	// All threads
	for (int i = 0; i < NUM_THREADS; i++)
	{
		thread[i] = CreateThread(NULL, 0, weightsThread, &tsk[i], 0, &tid[i]);
	}

	// Wait until threads are done
	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	// Close threads
	for (int i = 0; i < NUM_THREADS; i++)
	{
		CloseHandle(thread[i]);
	}

	printf("---CPU-time features : %.3f\n", timerGetS(1));
	
}



