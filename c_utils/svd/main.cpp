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

svd_entry userSVD[USER_NUM];
svd_entry movieSVD[MOVIE_NUM];
svd_entry userSVDT[USER_NUM];
svd_entry movieSVDT[MOVIE_NUM];
svd_entry userSVDTT[USER_NUM];
svd_entry movieSVDTT[MOVIE_NUM];
//
//
//std::vector<data_entry> train_data;
//std::vector<float> train_temp;

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
	memcpy(userSVDT, userSVD, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVDT, movieSVD, sizeof(svd_entry)*MOVIE_NUM);
	trainParallelIterationA(userSVD, movieSVD, l, train_dataA, train_tempA, train_sizeA);
	printf("---The iteration took %.3f\n", timerGetS(0));
	printf("---Valid RMSE: %.4f\n", newRMSE = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA));
	if (newRMSE > lastRMSE || newRMSE != newRMSE)
	{
		printf("***newRMSE > lastRMSE ; rollback; decrease learning rate\n");
		memcpy(userSVD, userSVDT, sizeof(svd_entry)*USER_NUM);
		memcpy(movieSVD, movieSVDT, sizeof(svd_entry)*MOVIE_NUM);
		l *= 0.8;
		lastRMSE = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA);
		return true;
	}
	else
	{
		l *= 1.005;
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
	memcpy(userSVDTT, userSVD, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVDTT, movieSVD, sizeof(svd_entry)*MOVIE_NUM);
	trainParallelIterationA(userSVD, movieSVD, l*1.5, train_dataA, train_tempA, train_sizeA);

	rmseUP = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA);
	
	memcpy(userSVD, userSVDTT, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVD, movieSVDTT, sizeof(svd_entry)*MOVIE_NUM);
	trainParallelIterationA(userSVD, movieSVD, l / 1.5, train_dataA, train_tempA, train_sizeA);
	
	rmseDOWN = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA);
	
	memcpy(userSVD, userSVDTT, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVD, movieSVDTT, sizeof(svd_entry)*MOVIE_NUM);
	trainParallelIterationA(userSVD, movieSVD, l, train_dataA, train_tempA, train_sizeA);
	
	rmse = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA);
	
	if (rmseUP < rmseDOWN && rmseUP < rmse)
		l *= 1.5;
	if (rmseDOWN < rmseUP && rmseDOWN < rmse)
			l /= 1.5;
	
	memcpy(userSVD, userSVDTT, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVD, movieSVDTT, sizeof(svd_entry)*MOVIE_NUM);
}

int main()
{
	FILE *f = fopen("log.csv", "wt");
	int Niter = 10;
	float l = 0.000015;
	float rmse = 100;

	timerStart(0);

	initSVD(userSVD, movieSVD);
	memcpy(userSVDT, userSVD, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVDT, movieSVD, sizeof(svd_entry)*MOVIE_NUM);

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
	printf("Valid RMSE: %.4f\n", rmse = RMSEA(userSVD, movieSVD, valid_dataA, valid_sizeA));
	printf("Computing train RMSE takes %.4f", timerGetS(2));
	printf("Probe RMSE: %.4f\n", rmse = RMSEA(userSVD, movieSVD, probe_dataA, probe_sizeA));

	timerStart(2);
	for (int i = 0; i < Niter; i++)
	{
		printf("\n\n#[%3d]  learning rate %.7f\n", i+1,l);
		if (!smartIteration(l, rmse))break;
		fprintf(f, "%d, %.7f\n", i + 1, rmse);
		if ((i+1) % 30 == 0) ChooseRate(l);
		if ((i + 1) % 5 == 0) printf("---Probe RMSE: %.4f\n", RMSEA(userSVD, movieSVD, probe_dataA, probe_sizeA));
	}
	fclose(f);
	printf("\n\n------------------------\n\n");
	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);
	saveSVD(userSVD, USER_NUM, "user.svd");
	saveSVD(userSVD, MOVIE_NUM, "movie.svd");
}