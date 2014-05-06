#include"types.h"

void	init_mod_kNN(Model* mod_kNN);
float	predict(Model * kNN,
			Biases * biases,
			UserStats *u,
			unsigned int * sp_R,
			unsigned int * sp_N,
			unsigned int * startpMatArray,
			unsigned int user_u,
			unsigned int movie_i,
			DataEntry *trainset);
float	computeRMSE(Model * kNN,
			Biases * biases,
			UserStats *u,
			unsigned int * sp_R,
			unsigned int * sp_N,
			unsigned int * spMatArray,
			DataEntry* trainset,
			DataEntry* testset,
			int testsize);

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
	);

void trainParallelIteration(Model* kNN,
	Biases * biases,
	UserStats* u,
	unsigned int * sp_R,
	unsigned int * sp_N,
	unsigned int * spMatArray,
	float l,
	DataEntry* trainset,
	float* temporary,
	int trainsize);