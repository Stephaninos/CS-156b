#include"types.h"

float predict(Model * kNN,
	UserStats *u,
	unsigned int * sp_R,
	unsigned int * sp_N,
	unsigned int * startpMatArray,
	unsigned int user_u,
	unsigned int movie_i,
	DataEntry *trainset);

void				init_mod_kNN(Model* mod_kNN);
float				computeRMSE(Model * kNN,
								UserStats *u,
								unsigned int * sp_R,
								unsigned int * sp_N,
								unsigned int * spMatArray,
								DataEntry* trainset,
								int trainsize,
								DataEntry* testset,
								int testsize);

void trainParallelIteration(Model* kNN,
	UserStats* u,
	unsigned int * sp_R,
	unsigned int * sp_N,
	unsigned int * spMatArray,
	float l,
	DataEntry* trainset,
	float* temporary,
	int trainsize);