#include <vector>
#include "io.h"
#include <Windows.h>
#include "types.h"
#include "timer.h"
#include "train.h"

char * um_train = "../../um/all.dta.train";
char * mu_train = "../../mu/all.dta.train";
char * um_probe = "../../um/all.dta.probe";
char * mu_probe = "../../mu/all.dta.probe";

//svd_entry userSVD[USER_NUM];
//svd_entry movieSVD[MOVIE_NUM];
//svd_entry userSVDT[USER_NUM];
//svd_entry movieSVDT[MOVIE_NUM];
//svd_entry userSVDTT[USER_NUM];
//svd_entry movieSVDTT[MOVIE_NUM];
model SVD;
model SVDT;
model SVDTT;
model SVDB;
//
//
//std::vector<data_entry> train_data;
//std::vector<float> train_temp;

//svd_entry userSVDB[USER_NUM];
//svd_entry movieSVDB[MOVIE_NUM];

//svd_entry *bestUserSVD;
//svd_entry *bestMovieSVD;

model* bestSVD;

float bestProbeRMSE = 100;

void keepBestProbeRMSE(float newRMSE)
{
	if (newRMSE < bestProbeRMSE)
	{
		bestProbeRMSE = newRMSE;
		bestSVD = &SVD;
	}
	else
	{
		if (bestSVD == &SVD)
		{
			memcpy(&SVDB, &SVDT, sizeof(model));
			bestSVD = &SVDB;
		}
	}
}


data_entry* train_dataA;
int train_sizeA;
float* train_tempA;

data_entry* probe_dataA;
int probe_sizeA;

data_entry* valid_dataA;
int valid_sizeA;

bool smartIteration(float & l,float &lastRMSE)
{
	float newRMSE;
	timerStart(0);
	memcpy(&SVDT, &SVD, sizeof(model));
	trainParallelIterationA(&SVD, l, train_dataA, train_tempA, train_sizeA);
	printf("---The iteration took %.3f\n", timerGetS(0));
	printf("---Valid RMSE: %.4f\n", newRMSE = RMSEA(&SVD, valid_dataA, valid_sizeA));
	if (newRMSE > lastRMSE || newRMSE != newRMSE)
	{
		printf("***newRMSE > lastRMSE ; rollback; decrease learning rate\n");
		memcpy(&SVD, &SVDT, sizeof(model));
		l *= LEARNING_DEC;
		lastRMSE = RMSEA(&SVD, valid_dataA, valid_sizeA);
		return true;
	}
	else
	{
		l *= LEARNING_INC;
		if (lastRMSE - newRMSE < THRESHOLD)
		{
			lastRMSE = newRMSE;
			return false;
		}
		lastRMSE = newRMSE;
		return true;
	}
}

void ChooseRate(float & l)
{
	float rmseUP = 0, rmseDOWN=0, rmse=0;
	memcpy(&SVDTT, &SVD, sizeof(model));
	trainParallelIterationA(&SVD, l*LEARNING_UP, train_dataA, train_tempA, train_sizeA);

	rmseUP = RMSEA(&SVD, valid_dataA, valid_sizeA);
	
	memcpy(&SVD, &SVDTT, sizeof(model));
	trainParallelIterationA(&SVD, l/LEARNING_DOWN, train_dataA, train_tempA, train_sizeA);

	rmseDOWN = RMSEA(&SVD, valid_dataA, valid_sizeA);
	
	memcpy(&SVD, &SVDTT, sizeof(model));
	trainParallelIterationA(&SVD, l, train_dataA, train_tempA, train_sizeA);

	rmse = RMSEA(&SVD, valid_dataA, valid_sizeA);
	
	if (rmseUP < rmseDOWN && rmseUP < rmse)
		l *= LEARNING_UP;
	if (rmseDOWN < rmseUP && rmseDOWN < rmse)
			l /= LEARNING_DOWN;
	
	memcpy(&SVD, &SVDTT, sizeof(model));
}

int main()
{
	saveParam();
	char name[1024];
	strcpy(name, RUN_NAME);
	strcat(name, ".log.csv");
	FILE * f = fopen(name, "wt");
	int Niter = ITERATION_NUM;
	float l = LEARNING_RATE;
	float rmse = 100;
	float probermse = 100;

	timerStart(0);

	initSVD(&SVD);
	memcpy(&SVDT, &SVD, sizeof(model));

	printf("SVD Initialization took %.3f\n", timerGetS(0));

	train_dataA = new data_entry[100000000];
	train_tempA = new float[100000000];
	probe_dataA = new data_entry[2000000];
	valid_dataA = new data_entry[2000000];

	train_sizeA = fillWithDataA(train_dataA, mu_train, 100000000);
	valid_sizeA = fillWithDataA(valid_dataA, mu_train, 100000000, 2);
	probe_sizeA = fillWithDataA(probe_dataA, mu_probe, 2000000);

	printf("Temporary size is %d\n", train_sizeA);
	timerStart(2);
	printf("Valid RMSE: %.4f\n", rmse = RMSEA(&SVD, valid_dataA, valid_sizeA));
	printf("Computing train RMSE takes %.4f\n", timerGetS(2));
	printf("Probe RMSE: %.4f\n", RMSEA(&SVD, probe_dataA, probe_sizeA));

	timerStart(2);
	for (int i = 0; i < Niter; i++)
	{
		printf("\n\n#[%3d]  learning rate %.7f\n", i+1,l);
		if (!smartIteration(l, rmse))break;
		fprintf(f, "%d, %.7f, %.7f\n", i + 1, rmse, probermse=RMSEA(&SVD, probe_dataA, probe_sizeA));
		keepBestProbeRMSE(probermse);
		if ((i+1) % TUNE_FREQ == 0) ChooseRate(l);
		if ((i + 1) % 5 == 0) printf("---Probe RMSE: %.4f\n", probermse);
		printf("---Estimate time left: %.3f m", (Niter - i - 1)*(timerGetS(2) / (i + 1)) / 60);
	}
	fclose(f);
	printf("\n\n------------------------\n\n");
	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);
	saveSVD2(&SVD, "user.svd2");
	saveSVD2(bestSVD, "movie_movie.svd2");
}