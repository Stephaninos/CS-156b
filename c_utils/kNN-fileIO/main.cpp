#include <vector>
#include "io.h"
#include <Windows.h>
#include <iostream>
#include "types.h"
#include "timer.h"
#include "train.h"

#include <ctime>
#include <cstring>
#include <sstream>

// Data
//# parameters
//# ==========
//# processed files
//# all.dta
//# all.idx
//# found 102416306 entries, out of which
//# train 1 : 94362233
//# train 2 : 1965045
//# train 3 : 1964391
//# train 6 : 0
//# probe : 1374739
//# test : 2749898

char*		mu_all	 = "../../../um/all.dta.bin";
char*		mu_train = "../../../um/all.dta.train";
char*		mu_probe = "../../../um/all.dta.probe";
char*		out_file = "out_userstats.bin";

DataEntry*	train_dataA;
int			train_sizeA;
float*		train_tempA;

DataEntry*	probe_dataA;
int			probe_sizeA;

DataEntry*	valid_dataA;
int			valid_sizeA;

UserStats*	u;

unsigned int * mat_startingPointsUM;
unsigned int * mat_startingPointsMatArray;

// mod_kNN data-structures

Model*		mod_kNN;
Model*		mod_kNN_temp;
Model*		mod_kNN_temp_temp;
Model*		mod_kNN_best_seen;

float		rmse_temp = 100;

// Keep track of best param set seen

float		bestProbeRMSE = 100;

//void keepBestProbeRMSE(float newRMSE)
//{
//	if (newRMSE < bestProbeRMSE)
//	{
//		bestProbeRMSE = newRMSE;
//		bestmod_kNN = &mod_kNN;
//	}
//	else
//	{
//		if (bestmod_kNN == &mod_kNN)
//		{
//			memcpy(&mod_kNNB, &mod_kNNT, sizeof(Model));
//			bestmod_kNN = &mod_kNNB;
//		}
//	}
//}




bool smartIteration(float & l, float &lastRMSE)
{
	float newRMSE;
	timerStart(0);
	memcpy(&mod_kNN_temp, &mod_kNN, sizeof(Model));

	// trainParallelIterationA(&mod_kNN, l, train_dataA, train_tempA, train_sizeA);
	
	printf("---The iteration took %.3f\n", timerGetS(0));
	printf("---Valid RMSE: %.4f\n", newRMSE = RMSEA(mod_kNN, u, mat_startingPointsUM, mat_startingPointsMatArray, train_dataA, train_sizeA, valid_dataA, valid_sizeA));
	if (newRMSE > lastRMSE || newRMSE != newRMSE)
	{
		printf("***newRMSE > lastRMSE ; rollback; decrease learning rate\n");
		memcpy(&mod_kNN, &mod_kNN_temp, sizeof(Model));
		l *= LEARNING_DEC;
		lastRMSE = RMSEA(mod_kNN, u, mat_startingPointsUM, mat_startingPointsMatArray, train_dataA, train_sizeA, valid_dataA, valid_sizeA);
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
//
//void ChooseRate(float & l)
//{
//	float rmseUP = 0, rmseDOWN=0, rmse=0;
//	memcpy(&SVDTT, &SVD, sizeof(Model));
//	trainParallelIterationA(&SVD, l*LEARNING_UP, train_dataA, train_tempA, train_sizeA);
//
//	rmseUP = RMSEA(&SVD, valid_dataA, valid_sizeA);
//	
//	memcpy(&SVD, &SVDTT, sizeof(Model));
//	trainParallelIterationA(&SVD, l/LEARNING_DOWN, train_dataA, train_tempA, train_sizeA);
//
//	rmseDOWN = RMSEA(&SVD, valid_dataA, valid_sizeA);
//	
//	memcpy(&SVD, &SVDTT, sizeof(Model));
//	trainParallelIterationA(&SVD, l, train_dataA, train_tempA, train_sizeA);
//
//	rmse = RMSEA(&SVD, valid_dataA, valid_sizeA);
//	
//	if (rmseUP < rmseDOWN && rmseUP < rmse)
//		l *= LEARNING_UP;
//	if (rmseDOWN < rmseUP && rmseDOWN < rmse)
//			l /= LEARNING_DOWN;
//	
//	memcpy(&SVD, &SVDTT, sizeof(Model));
//}
//

// Note: just store these numbers in a file to make this faster?
bool getStartingPointsInUM(unsigned int * mat, UserStats *u){
	int sum = 0;
	for (int k = 0; k < USER_NUM; k++){
		mat[k] = sum;
		sum += (*u).allRatedMoviesWithRatingByUserSize_R[k];
	};
	return 1;
};


// To look up where the ratings for user i start in um/all.dta.train USER-MOVIE SORTED!!!
bool getStartingPointsInMatArray(unsigned int * mat){
	int sum = 0;	
	mat[0]	= 0;
	for (int k = 2; k <= MOVIE_NUM; k++){
		for (int j = 1; j < k; j++){
			sum += MOVIE_NUM - j;
		}
		mat[k - 1] = sum; // 0-indexed
		sum = 0;
	};

	//for (int kk = MOVIE_NUM - 10; kk < MOVIE_NUM; kk++){
	//	printf("Position: %u", mat[kk]);
	//};
	//std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	//std::cin.ignore(1);
		
	return 1;
};

void main(){
	
	// Only run this if userstats.bin is not there yet.
	// countStatsAndWriteToFile(mu_all, out_file);
	
	// Start time of run
	std::time_t t = std::time(NULL);
	char starttime[sizeof(t)];
	std::strftime(starttime, sizeof(starttime), "%d-%m-%H%M", std::localtime(&t));
	
	timerStart(0);

	// Fill UserStats from .bin file
	u = new UserStats;
	fillUserStats(out_file, u);	
	printf("Filling UserStats took %.3f\n", timerGetS(0));

	timerStart(1);
	// Get array with startingpoints in UM data to fetch ratings later
	mat_startingPointsUM		= new unsigned int[USER_NUM];
	mat_startingPointsMatArray	= new unsigned int[MOVIE_NUM];
	getStartingPointsInUM(mat_startingPointsUM, u); 
	getStartingPointsInMatArray(mat_startingPointsMatArray);

	printf("Generating indices took %.10f\n", timerGetS(1));	

	printf("Blank line read! Check: User 1: %u \n", (*u).allRatedMoviesByUser_N[(*u).allRatedMoviesByUserSize_N[0]]);

	///////////////////////////////////////////////////
	//	Check counts
	///////////////////////////////////////////////////
/*
	for (int i = 0; i < 100; i++){		
		printf("User %u has %u ratings \n", i, (*u).allRatedMoviesByUserSize_N[i]);				
	};

	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	for (int i = 0; i < 100; i++){
		printf("User %u saw movie %u \n", i, (*u).allRatedMoviesWithRatingByUser_R[i]);
	};

	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);
*/
	///////////////////////////////////////////////////
	//	End check counts
	///////////////////////////////////////////////////

	// Save parameters
	saveParam(starttime);

	// Start log file for RMSEs
	char name_logfile[1024];
	strcpy(name_logfile, starttime);
	strcat(name_logfile, RUN_NAME);
	strcat(name_logfile, ".log.csv");
	
	FILE * f		= fopen(name_logfile, "wt");
	int Niter		= ITERATION_NUM;
	float l			= LEARNING_RATE;
	float rmse		= 100;
	float probermse = 100;
		
	timerStart(0);
	
	// Initialize Model 
	mod_kNN				= new Model;
	mod_kNN_temp		= new Model;
	// mod_kNN_temp_temp	= new Model;
	// mod_kNN_best_seen	= new Model;

	init_mod_kNN(mod_kNN);
	init_mod_kNN(mod_kNN_temp);
	// memcpy(&mod_kNN_temp, &mod_kNN, sizeof(Model));
	
	printf("mod_kNN Initialization took %.3f\n", timerGetS(0));
	
	train_dataA = new DataEntry[TOTAL_NUM_RATINGS];
	train_tempA = new float[TOTAL_NUM_RATINGS];
	probe_dataA = new DataEntry[NUM_PROBE_RATINGS];
	valid_dataA = new DataEntry[NUM_VALID_RATINGS];

	train_sizeA = fillWithData(train_dataA, mu_train, NUM_TRAIN_RATINGS + NUM_VALID_RATINGS);
	valid_sizeA = fillWithData(valid_dataA, mu_train, NUM_TRAIN_RATINGS + NUM_VALID_RATINGS, 2);
	probe_sizeA = fillWithData(probe_dataA, mu_probe, NUM_PROBE_RATINGS);
	
	printf("Temporary size is %d\n", train_sizeA);
	timerStart(2);
	printf("Valid RMSE: %.4f\n", rmse = RMSEA(	mod_kNN, 
												u, 
												mat_startingPointsUM, 
												mat_startingPointsMatArray, 
												train_dataA, 
												train_sizeA, 
												valid_dataA, 
												valid_sizeA));
	printf("Computing valid RMSE takes %.4f\n", timerGetS(2));
	printf("Probe RMSE: %.4f\n",		RMSEA(	mod_kNN, 
												u, 
												mat_startingPointsUM, 
												mat_startingPointsMatArray, 
												train_dataA, 
												train_sizeA, 
												probe_dataA, 
												probe_sizeA));	
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	//	timerStart(2);
	//	for (int i = 0; i < Niter; i++)
	//	{
	//		printf("\n\n#[%3d]  learning rate %.7f\n", i+1,l);
	//		if (!smartIteration(l, rmse))break;
	//		fprintf(f, "%d, %.7f, %.7f\n", i + 1, rmse, probermse=RMSEA(&SVD, probe_dataA, probe_sizeA));
	//		keepBestProbeRMSE(probermse);
	//		if ((i+1) % TUNE_FREQ == 0) ChooseRate(l);
	//		if ((i + 1) % 5 == 0) printf("---Probe RMSE: %.4f\n", probermse);
	//		printf("---Estimate time left: %.3f m", (Niter - i - 1)*(timerGetS(2) / (i + 1)) / 60);
	//	}
	//	fclose(f);
	//	printf("\n\n------------------------\n\n");
	//	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);
	//	saveSVD2(&SVD, "22-04-0001all.svd2");
	//	saveSVD2(bestSVD, "22-04-0001all-best.svd2");

	
	
	
	
	
	// Loop converged or we hit max iters
	double validRMSE = RMSEA(mod_kNN,
							u,
							mat_startingPointsUM,
							mat_startingPointsMatArray,
							train_dataA,
							train_sizeA,
							valid_dataA,
							valid_sizeA);
	double probeRMSE = RMSEA(mod_kNN,
							u,
							mat_startingPointsUM,
							mat_startingPointsMatArray,
							train_dataA,
							train_sizeA,
							probe_dataA,
							probe_sizeA);


	char mbstr[100];
	char mmbstr[100];
	std::strftime(mbstr, sizeof(mbstr), "%d-%m-%H%M", std::localtime(&t));
	std::strftime(mmbstr, sizeof(mmbstr), "%d-%m-%H%M", std::localtime(&t));

	std::string str1 = std::to_string(validRMSE);
	std::string str2 = std::to_string(probeRMSE);
	strcat(mbstr, str1.c_str());
	strcat(mbstr, str2.c_str());
	strcat(mmbstr, str1.c_str());
	strcat(mmbstr, str2.c_str());

	// Write mod_kNN to file

	fclose(f);
	printf("\n\n------------------------\n\n");
	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);
	save_kNN(mod_kNN, "22-04-0001all.svd2");
	
	//	saveSVD2(bestSVD, "22-04-0001all-best.svd2");
	
	delete[] mat_startingPointsMatArray;
	delete[] mat_startingPointsUM;
	delete u;
	delete[] train_dataA;
	delete[] train_tempA;
	delete[] probe_dataA;
	delete[] valid_dataA;

}



//int main()
//{
//	saveParam();
//	char name[1024];
//	strcpy(name, RUN_NAME);
//	strcat(name, ".log.csv");
//	FILE * f = fopen(name, "wt");
//	int Niter = ITERATION_NUM;
//	float l = LEARNING_RATE;
//	float rmse = 100;
//	float probermse = 100;
//
//	timerStart(0);
//
//	init_mod_kNN(&SVD);
//	memcpy(&SVDT, &SVD, sizeof(Model));
//
//	printf("SVD Initialization took %.3f\n", timerGetS(0));
//
//	train_dataA = new DataEntry[100000000];
//	train_tempA = new float[100000000];
//	probe_dataA = new DataEntry[2000000];
//	valid_dataA = new DataEntry[2000000];
//
//	train_sizeA = fillWithData(train_dataA, mu_train, 100000000);
//	valid_sizeA = fillWithData(valid_dataA, mu_train, 100000000, 2);
//	probe_sizeA = fillWithData(probe_dataA, mu_probe, 2000000);
//
//	printf("Temporary size is %d\n", train_sizeA);
//	timerStart(2);
//	printf("Valid RMSE: %.4f\n", rmse = RMSEA(&SVD, valid_dataA, valid_sizeA));
//	printf("Computing train RMSE takes %.4f\n", timerGetS(2));
//	printf("Probe RMSE: %.4f\n", RMSEA(&SVD, probe_dataA, probe_sizeA));
//
//	timerStart(2);
//	for (int i = 0; i < Niter; i++)
//	{
//		printf("\n\n#[%3d]  learning rate %.7f\n", i+1,l);
//		if (!smartIteration(l, rmse))break;
//		fprintf(f, "%d, %.7f, %.7f\n", i + 1, rmse, probermse=RMSEA(&SVD, probe_dataA, probe_sizeA));
//		keepBestProbeRMSE(probermse);
//		if ((i+1) % TUNE_FREQ == 0) ChooseRate(l);
//		if ((i + 1) % 5 == 0) printf("---Probe RMSE: %.4f\n", probermse);
//		printf("---Estimate time left: %.3f m", (Niter - i - 1)*(timerGetS(2) / (i + 1)) / 60);
//	}
//	fclose(f);
//	printf("\n\n------------------------\n\n");
//	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);
//	saveSVD2(&SVD, "22-04-0001all.svd2");
//	saveSVD2(bestSVD, "22-04-0001all-best.svd2");
//}