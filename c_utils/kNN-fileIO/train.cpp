#include <Windows.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "types.h"
#include "timer.h"

//
//struct task
//{
//	float l;
//	DataEntry * data;
//	float * temp;
//	svd_entry * users;
//	svd_entry * movies;
//	int first;
//	int last;
//};
//
//struct task2
//{
//	float l;
//	DataEntry * data;
//	float * temp;
//	Model * svd;
//	int first;
//	int last;
//};


static inline float predict(Model * kNN, UserStats *u, unsigned int * startp, unsigned int * startpMatArray, unsigned int a, unsigned int b, DataEntry *trainset)
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

float RMSEA(Model * kNN, UserStats *u, unsigned int * sp, unsigned int * spMatArray, DataEntry* trainset, int trainsize, DataEntry* testset, int testsize)
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
	
//
//void trainIterationA(svd_entry * users, svd_entry * movies, float l, DataEntry* trainset, float* temporary, int trainsize)
//{
//	float t,h = 0;
//	svd_entry *u, *m;
//	DataEntry *tt;
//	timerStart(1);
//	for (int i = 0; i < trainsize; i++)
//	{
//		tt = &trainset[i];
//		temporary[i] = l*(tt->rating - predict(users, movies, tt->user_ptr, tt->movie_ptr));
//	}
//	printf("---errors:%.3f\n", timerGetS(1));
//	for (int i = 0; i < SVD_dim; i++)
//	{
//		timerStart(1);
//		for (int j = 0; j < trainsize; j++)
//		{
//			u = &users[trainset[j].user_ptr];
//			m = &movies[trainset[j].movie_ptr];
//			h = u->params[i];
//			t = temporary[j];
//			u->params[i] +=  t*m->params[i];
//			m->params[i] += t*h;
//		}
//	}
//}
//
//DWORD WINAPI featureThread(LPVOID param)
//{
//	float t, h = 0;
//	float *u, *m;
//	task p = *(task*)param;
//
//	for (int j = p.first; j <= p.last; j++)
//	{
//		for (int i = 0; i < SVD_dim; i++)
//		{
//			u = &p.users[p.data[j].user_ptr].params[i];
//			m = &p.movies[p.data[j].movie_ptr].params[i];
//			h = *u;
//			t = p.temp[j];
//			*u += t*(*m) - REGULARIZATION_CONST_U*p.l*(*u);
//			*m += t*h - REGULARIZATION_CONST_M*p.l*(*m);
//		}
//	}
//	return 0;
//}
//
//DWORD WINAPI featureThread2(LPVOID param)
//{
//	float t, h = 0;
//	float *u, *m;
//	task2 p = *(task2*)param;
//
//	for (int j = p.first; j <= p.last; j++)
//	{
//		t = p.l*p.temp[j];
//		for (int i = 0; i < SVD_dim; i++)
//		{
//			u = &p.svd->userSVD[p.data[j].user_ptr].params[i];
//			m = &p.svd->movieSVD[p.data[j].movie_ptr].params[i];
//			h = *u;
//			*u += t*(*m) - REGULARIZATION_CONST_U*p.l*(*u);
//			*m += t*h - REGULARIZATION_CONST_M*p.l*(*m);
//		}
//		p.svd->bu[p.data[j].user_ptr] += B_LEARNING_RATIO*t;
//		p.svd->bm[p.data[j].movie_ptr] += B_LEARNING_RATIO*t;
//	}
//	return 0;
//}
//
//DWORD WINAPI errorThread(LPVOID param)
//{
//	task t = *(task*)param;
//	DataEntry *tt;
//	for (int i = t.first; i <= t.last; i++)
//	{
//		tt = &t.data[i];
//		t.temp[i] = t.l*(tt->rating - predict(t.users, t.movies, tt->user_ptr, tt->movie_ptr));
//	}
//	return 0;
//}
//
//DWORD WINAPI errorThread2(LPVOID param)
//{
//	task2 t = *(task2*)param;
//	DataEntry *tt;
//	for (int i = t.first; i <= t.last; i++)
//	{
//		tt = &t.data[i];
//		t.temp[i] = (tt->rating - predict(t.svd, tt->user_ptr, tt->movie_ptr));
//	}
//	return 0;
//}
//
//void trainParallelIterationA(svd_entry * users, svd_entry * movies, float l, DataEntry* trainset, float* temporary, int trainsize)
//{
//	float t, h = 0;
//	float *u, *m;
//	DataEntry *tt;
//	timerStart(1);
//	DWORD tid[NUM_THREADS];
//	task tsk[NUM_THREADS];
//	HANDLE thread[NUM_THREADS];
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		tsk[i].first = (trainsize / NUM_THREADS)*i;
//		tsk[i].last = (trainsize / NUM_THREADS)*(i+1)-1;
//		tsk[i].data = trainset;
//		tsk[i].temp = temporary;
//		tsk[i].l = l;
//		tsk[i].users = users;
//		tsk[i].movies = movies;
//		thread[i] = CreateThread(NULL, 0, errorThread, &tsk[i], 0, &tid[i]);
//	}
//
//	for (int i = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); i < trainsize; i++)
//	{
//		tt = &trainset[i];
//		temporary[i] = l*(tt->rating - predict(users, movies, tt->user_ptr, tt->movie_ptr));
//	}
//
//	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		CloseHandle(thread[i]);
//	}
//	printf("---errors:%.3f\n", timerGetS(1));
//	
//	timerStart(1);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		thread[i] = CreateThread(NULL, 0, featureThread, &tsk[i], 0, &tid[i]);
//	}	
//
//	for (int j = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); j < trainsize; j++)
//	{
//		for (int i = 0; i < SVD_dim; i++)
//		{
//			u = &users[trainset[j].user_ptr].params[i];
//			m = &movies[trainset[j].movie_ptr].params[i];
//			h = *u;
//			t = temporary[j];
//			*u += t*(*m) - REGULARIZATION_CONST_U*l*(*u);
//			*m += t*h - REGULARIZATION_CONST_M*l*(*m);
//		}
//	}
//
//	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		CloseHandle(thread[i]);
//	}
//
//	printf("---features:%.3f\n", timerGetS(1));
//
//}
//
//void trainParallelIterationA(Model * svd, float l, DataEntry* trainset, float* temporary, int trainsize)
//{
//	float t, h = 0;
//	float *u, *m;
//	DataEntry *tt;
//	timerStart(1);
//	DWORD tid[NUM_THREADS];
//	task2 tsk[NUM_THREADS];
//	HANDLE thread[NUM_THREADS];
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		tsk[i].first = (trainsize / NUM_THREADS)*i;
//		tsk[i].last = (trainsize / NUM_THREADS)*(i + 1) - 1;
//		tsk[i].data = trainset;
//		tsk[i].temp = temporary;
//		tsk[i].l = l;
//		tsk[i].svd = svd;
//		thread[i] = CreateThread(NULL, 0, errorThread2, &tsk[i], 0, &tid[i]);
//	}
//
//	for (int i = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); i < trainsize; i++)
//	{
//		tt = &trainset[i];
//		temporary[i] = (tt->rating - predict(svd, tt->user_ptr, tt->movie_ptr));
//	}
//
//	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		CloseHandle(thread[i]);
//	}
//	printf("---errors:%.3f\n", timerGetS(1));
//
//	timerStart(1);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		thread[i] = CreateThread(NULL, 0, featureThread2, &tsk[i], 0, &tid[i]);
//	}
//
//	for (int j = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); j < trainsize; j++)
//	{
//		t = l*temporary[j];
//		for (int i = 0; i < SVD_dim; i++)
//		{
//			u = &svd->userSVD[trainset[j].user_ptr].params[i];
//			m = &svd->movieSVD[trainset[j].movie_ptr].params[i];
//			h = *u;
//			*u += t*(*m) - REGULARIZATION_CONST_U*l*(*u);
//			*m += t*h - REGULARIZATION_CONST_M*l*(*m);
//		}
//		svd->bu[trainset[j].user_ptr] += B_LEARNING_RATIO*t;
//		svd->bm[trainset[j].movie_ptr] += B_LEARNING_RATIO*t;
//	}
//
//	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);
//
//	for (int i = 0; i < NUM_THREADS - 1; i++)
//	{
//		CloseHandle(thread[i]);
//	}
//
//	printf("---features:%.3f\n", timerGetS(1));
//
//}
//


