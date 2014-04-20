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


std::vector<data_entry> train_data;
std::vector<float> train_temp;

data_entry* train_dataA;
int train_sizeA;
float* train_tempA;

data_entry* probe_dataA;
int probe_sizeA;

bool smartIteration(float & l,float &lastRMSE)
{
	float newRMSE;
	timerStart(0);
	memcpy(userSVDT, userSVD, sizeof(svd_entry)*USER_NUM);
	memcpy(movieSVDT, movieSVD, sizeof(svd_entry)*MOVIE_NUM);
	trainParallelIterationA(userSVD, movieSVD, l, train_dataA, train_tempA, train_sizeA);
	printf("One iteration takes %.3f\n", timerGetS(0));
	printf("Probe RMSE: %.4f\n", newRMSE=RMSEA(userSVD, movieSVD, probe_dataA, probe_sizeA));
	if (newRMSE > lastRMSE || newRMSE != newRMSE)
	{
		printf("newRMSE > lastRMSE ; rollback; decrease learning rate\n");
		memcpy(userSVD, userSVDT, sizeof(svd_entry)*USER_NUM);
		memcpy(movieSVD, movieSVDT, sizeof(svd_entry)*MOVIE_NUM);
		l *= 0.9;
		return true;
	}
	else
	{
		l *= 1.1;
		if (lastRMSE - newRMSE < THRESHOLD)
		{
			lastRMSE = newRMSE;
			return false;
		}
		lastRMSE = newRMSE;
		return true;
	}
}

int main()
{

	float l = 0.002;
	float rmse = 100;

	timerStart(0);
	initSVD(userSVD, movieSVD);
	printf("SVD Initialization took %.3f\n", timerGetS(0));

	train_dataA = new data_entry[100000000];
	train_tempA = new float[100000000];
	probe_dataA = new data_entry[2000000];

	train_sizeA=fillWithDataA(train_dataA, mu_train, 100000000);
	probe_sizeA=fillWithDataA(probe_dataA, mu_probe, 2000000);

	printf("Temporary size is %d\n", train_sizeA);
	printf("Probe RMSE: %.4f\n", rmse=RMSEA(userSVD, movieSVD, probe_dataA, probe_sizeA));

	for (int i = 0; i < 100; i++)
	{
		if(!smartIteration(l, rmse))break;
	}
}