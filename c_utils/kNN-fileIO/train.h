#include"types.h"

bool				getStartingPointsInMatArray(unsigned int * mat);
static inline float predict(Model * kNN, UserStats *u, unsigned int * sp, unsigned int * spMatArray, unsigned int a, unsigned int b, DataEntry *trainset);
void				init_mod_kNN(Model* mod_kNN);
float				computeRMSE(Model * kNN, UserStats *u, unsigned int * sp, unsigned int * spMatArray, DataEntry* trainset, int trainsize, DataEntry* testset, int size);

void trainParallelIteration(Model* kNN,
	UserStats* u,
	unsigned int * sp,
	unsigned int * sp_N,
	unsigned int * spMatArray,
	float l,
	DataEntry* trainset,
	float* temporary,
	int trainsize);