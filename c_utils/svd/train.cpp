#include <vector>
#include <Windows.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "types.h"
#include "timer.h"

struct task
{
	float l;
	data_entry * data;
	float * temp;
	svd_entry * users;
	svd_entry * movies;
	int first;
	int last;
};

static inline float predict(svd_entry * users, svd_entry * movies, unsigned int a, unsigned int b)
{
	float s = 0;
	for (int i = 0; i < SVD_dim; i++)
	{
		s += users[a].params[i] * movies[b].params[i];
	}
	return s;
}

void trainIteration(svd_entry * users, svd_entry * movies, float l, std::vector<data_entry>& trainset, std::vector<float>& temporary)
{
	float h = 0;
	timerStart(1);
	for (int i = 0; i < trainset.size(); i++)
	{
		temporary[i] = l*(trainset[i].rating - predict(users, movies, trainset[i].user_ptr, trainset[i].movie_ptr));
	}
	printf("errors:%.3f\n", timerGetS(1));
	for (int i = 0; i < SVD_dim; i++)
	{
		timerStart(1);
		for (int j = 0; j < trainset.size(); j++)
		{
			h = users[trainset[j].user_ptr].params[i];
			users[trainset[j].user_ptr].params[i] += temporary[j]*movies[trainset[j].movie_ptr].params[i];
			movies[trainset[j].movie_ptr].params[i] += temporary[j]*h;
		}
		printf("time for feature %d : %.3f\n", i, timerGetS(1));
	}
}

void trainIterationA(svd_entry * users, svd_entry * movies, float l, data_entry* trainset, float* temporary, int trainsize)
{
	float t,h = 0;
	svd_entry *u, *m;
	data_entry *tt;
	timerStart(1);
	for (int i = 0; i < trainsize; i++)
	{
		tt = &trainset[i];
		temporary[i] = l*(tt->rating - predict(users, movies, tt->user_ptr, tt->movie_ptr));
	}
	printf("---errors:%.3f\n", timerGetS(1));
	for (int i = 0; i < SVD_dim; i++)
	{
		timerStart(1);
		for (int j = 0; j < trainsize; j++)
		{
			u = &users[trainset[j].user_ptr];
			m = &movies[trainset[j].movie_ptr];
			h = u->params[i];
			t = temporary[j];
			u->params[i] +=  t*m->params[i];
			m->params[i] += t*h;
		}
	}
}

DWORD WINAPI featureThread(LPVOID param)
{
	float t, h = 0;
	float *u, *m;
	task p = *(task*)param;

	for (int j = p.first; j <= p.last; j++)
	{
		for (int i = 0; i < SVD_dim; i++)
		{
			u = &p.users[p.data[j].user_ptr].params[i];
			m = &p.movies[p.data[j].movie_ptr].params[i];
			h = *u;
			t = p.temp[j];
			*u += t*(*m) - REGULARIZATION_CONST_U*p.l*(*u);
			*m += t*h - REGULARIZATION_CONST_M*p.l*(*m);
		}
	}
	return 0;
}

DWORD WINAPI errorThread(LPVOID param)
{
	task t = *(task*)param;
	data_entry *tt;
	for (int i = t.first; i <= t.last; i++)
	{
		tt = &t.data[i];
		t.temp[i] = t.l*(tt->rating - predict(t.users, t.movies, tt->user_ptr, tt->movie_ptr));
	}
	return 0;
}

void trainParallelIterationA(svd_entry * users, svd_entry * movies, float l, data_entry* trainset, float* temporary, int trainsize)
{
	float t, h = 0;
	float *u, *m;
	data_entry *tt;
	timerStart(1);
	DWORD tid[NUM_THREADS];
	task tsk[NUM_THREADS];
	HANDLE thread[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		tsk[i].first = (trainsize / NUM_THREADS)*i;
		tsk[i].last = (trainsize / NUM_THREADS)*(i+1)-1;
		tsk[i].data = trainset;
		tsk[i].temp = temporary;
		tsk[i].l = l;
		tsk[i].users = users;
		tsk[i].movies = movies;
		thread[i] = CreateThread(NULL, 0, errorThread, &tsk[i], 0, &tid[i]);
	}

	for (int i = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); i < trainsize; i++)
	{
		tt = &trainset[i];
		temporary[i] = l*(tt->rating - predict(users, movies, tt->user_ptr, tt->movie_ptr));
	}

	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}
	printf("---errors:%.3f\n", timerGetS(1));
	
	timerStart(1);

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		thread[i] = CreateThread(NULL, 0, featureThread, &tsk[i], 0, &tid[i]);
	}	

	for (int j = (trainsize / NUM_THREADS)*(NUM_THREADS - 1); j < trainsize; j++)
	{
		for (int i = 0; i < SVD_dim; i++)
		{
			u = &users[trainset[j].user_ptr].params[i];
			m = &movies[trainset[j].movie_ptr].params[i];
			h = *u;
			t = temporary[j];
			*u += t*(*m) - REGULARIZATION_CONST_U*l*(*u);
			*m += t*h - REGULARIZATION_CONST_M*l*(*m);
		}
	}

	WaitForMultipleObjects(NUM_THREADS, thread, true, INFINITE);

	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		CloseHandle(thread[i]);
	}

	printf("---features:%.3f\n", timerGetS(1));

}

void initSVD(svd_entry * users, svd_entry * movies)
{
	srand(time(NULL));
	for (int p = 0; p < SVD_dim; p++)
	{
		for (int i = 0; i < USER_NUM; i++)
		{
			users[i].params[p] = 0.1*float(rand()) / RAND_MAX;
		}
		for (int i = 0; i < MOVIE_NUM; i++)
		{
			movies[i].params[p] = 0.1*float(rand()) / RAND_MAX;
		}
	}
}

float RMSEA(svd_entry * users, svd_entry * movies, data_entry* testset, int size)
{
	double sum2=0;
	double p;
	for (int i = 0; i < size; i++)
	{
		p = predict(users, movies, testset[i].user_ptr, testset[i].movie_ptr);
		sum2 += (p - testset[i].rating)*(p - testset[i].rating);
	}
	return sqrt(sum2 / size);
}