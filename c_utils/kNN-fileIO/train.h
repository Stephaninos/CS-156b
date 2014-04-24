#include"types.h"

// unsigned int		getStartingPointsInUM(unsigned int * mat, UserStats *u);
// bool				getStartingPointsInMatArray(unsigned int * mat);
static inline float predict(Model * kNN, UserStats *u, unsigned int * sp, unsigned int * spMatArray, unsigned int a, unsigned int b, DataEntry *trainset);
void				init_mod_kNN(Model* mod_kNN);
float				RMSEA(Model * kNN, UserStats *u, unsigned int * sp, unsigned int * spMatArray, DataEntry* trainset, int trainsize, DataEntry* testset, int size);

//void trainIterationA(svd_entry * users, svd_entry * movies, float l, DataEntry* trainset, float* temporary, int size);
//void trainParallelIterationA(svd_entry * users, svd_entry * movies, float l, DataEntry* trainset, float* temporary, int size);
//void trainParallelIterationA(Model * svd, float l, DataEntry* trainset, float* temporary, int size);

