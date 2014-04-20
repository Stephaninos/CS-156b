#include<vector>
#include"types.h"

void trainIteration(svd_entry * users, svd_entry * movies, float l, std::vector<data_entry>& trainset, std::vector<float>& temporary);
void trainIterationA(svd_entry * users, svd_entry * movies, float l, data_entry* trainset, float* temporary, int size);
void trainParallelIterationA(svd_entry * users, svd_entry * movies, float l, data_entry* trainset, float* temporary, int size);
void initSVD(svd_entry * users, svd_entry * movies);
float RMSEA(svd_entry * users, svd_entry * movies, data_entry* testset, int size);
