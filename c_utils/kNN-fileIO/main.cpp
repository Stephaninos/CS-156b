#include <vector>
#include "io.h"
#include <Windows.h>
#include <iostream>
#include "types.h"
#include "timer.h"
#include "train.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <ctime>
#include <cstring>
#include <sstream>

// Data

// char*		mu_all	 = "../../../um/all.dta.bin";
// char*		mu_train = "../../../um/all.dta.train";


char*		mu_all	 = "../../../um/all.dta.allbut1";
char*		mu_train = "../../../um/all.dta.train23";

//char*		mu_all	 = "../../../um/all.dta.bin";
//char*		mu_train = "../../../um/all.dta.train123";
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

unsigned int * mat_startingPointsUM_R;
unsigned int * mat_startingPointsUM_N;
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



// Don't call computeRMSE again in smartIteration, store value and restore

bool smartIteration(Model *mod_kNN, Model *mod_kNN_temp, float & l, float &lastRMSE)
{
	float newRMSE;
	float rmse_lastrun = lastRMSE;

	timerStart(0);
	memcpy(mod_kNN_temp, mod_kNN, sizeof(Model));
	
	trainParallelIteration(mod_kNN,
		u,
		mat_startingPointsUM_R,
		mat_startingPointsUM_N,
		mat_startingPointsMatArray,
		l,
		train_dataA,
		train_tempA,
		train_sizeA);

	printf("---The iteration took: %.3f\n", timerGetS(0));
	printf("---\n");
	printf("---Train RMSE:      >> %.4f << \n", newRMSE = computeRMSE(mod_kNN, u, mat_startingPointsUM_R, mat_startingPointsUM_N, mat_startingPointsMatArray, train_dataA, train_dataA, train_sizeA));

	printf("---\n");
	printf("---Valid RMSE:      >> %.4f << \n", newRMSE = computeRMSE(mod_kNN, u, mat_startingPointsUM_R, mat_startingPointsUM_N, mat_startingPointsMatArray, train_dataA, valid_dataA, valid_sizeA));
	printf("---\n");
	if (newRMSE > lastRMSE || newRMSE != newRMSE)
	{
		printf("***newRMSE > lastRMSE ; rollback; decrease learning rate\n");
		memcpy(mod_kNN, mod_kNN_temp, sizeof(Model));
		l *= LEARNING_DEC;
		lastRMSE = rmse_lastrun;
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
//	trainParallelIteration(&SVD, l*LEARNING_UP, train_dataA, train_tempA, train_sizeA);
//
//	rmseUP = computeRMSE(&SVD, valid_dataA, valid_sizeA);
//	
//	memcpy(&SVD, &SVDTT, sizeof(Model));
//	trainParallelIteration(&SVD, l/LEARNING_DOWN, train_dataA, train_tempA, train_sizeA);
//
//	rmseDOWN = computeRMSE(&SVD, valid_dataA, valid_sizeA);
//	
//	memcpy(&SVD, &SVDTT, sizeof(Model));
//	trainParallelIteration(&SVD, l, train_dataA, train_tempA, train_sizeA);
//
//	rmse = computeRMSE(&SVD, valid_dataA, valid_sizeA);
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
bool getStartingPointsInUM_R(unsigned int * mat, UserStats *u){
	int sum = 0;
	for (int k = 0; k < USER_NUM; k++){
		mat[k] = sum;
		sum += (*u).nMovRated_R[k];
	};
	return 1;
};

bool getStartingPointsInUM_N(unsigned int * mat, UserStats *u){
	int sum = 0;
	for (int k = 0; k < USER_NUM; k++){
		mat[k] = sum;
		sum += (*u).nMovSeen_N[k];
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
	

	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	// Turn On (OR) - Keep freed memory blocks in the
	// heap's linked list and mark them as freed
	tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;

	// Turn on memory checking at each heap operation
	tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;

	// Set the new state for the flag
	_CrtSetDbgFlag(tmpFlag);




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
	mat_startingPointsUM_R		= new unsigned int[USER_NUM];
	mat_startingPointsUM_N		= new unsigned int[USER_NUM];
	mat_startingPointsMatArray	= new unsigned int[MOVIE_NUM];
	getStartingPointsInUM_R(mat_startingPointsUM_R, u); 
	getStartingPointsInUM_N(mat_startingPointsUM_N, u);
	getStartingPointsInMatArray(mat_startingPointsMatArray);

	printf("Generating indices took %.10f\n", timerGetS(1));	
		
	///////////////////////////////////////////////////
	//	Check counts
	///////////////////////////////////////////////////
/*
	for (int i = 0; i < 100; i++){		
		printf("sizeN[%u] = %u \n", i, (*u).nMovSeen_N[i]);				
	};

	for (int i = 0; i < 100; i++){
		printf("N[%u] = %u \n", i, (*u).idMovSeen_N[i]);
	};
	
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	for (int i = 0; i < 100; i++){
		printf("sizeR[%u] = %u \n", i, (*u).nMovRated_R[i]);
	};

	for (int i = 0; i < 100; i++){
		printf("R[%u] = %u \n", i, (*u).idMovRated_R[i]);
	};

	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);
*/
	///////////////////////////////////////////////////
	//	End check counts
	///////////////////////////////////////////////////

	// Save parameters
	saveParam(&t);

	// Start log file for RMSEs
	char name_logfile[1024];
	strcpy(name_logfile, starttime);	
	strcat(name_logfile, ".log.csv");
	
	FILE * f		= fopen(name_logfile, "wt");
	int Niter		= ITERATION_NUM;
	float l			= LEARNING_RATE;
	float rmse		= 100;
	float probermse = 100;
		
	timerStart(0);
	
	// Initialize Model 
	mod_kNN					= new Model;
	mod_kNN_temp			= new Model;
	// mod_kNN_temp_temp	= new Model;
	// mod_kNN_best_seen	= new Model;

	init_mod_kNN(mod_kNN);
	init_mod_kNN(mod_kNN_temp);
	// memcpy(&mod_kNN_temp, &mod_kNN, sizeof(Model));
	
	printf("mod_kNN Initialization took %.3f\n", timerGetS(0));
	
	train_dataA = new DataEntry[NUM_TRAIN_RATINGS];
	train_tempA = new float[NUM_TRAIN_RATINGS];
	probe_dataA = new DataEntry[NUM_PROBE_RATINGS];
	valid_dataA = new DataEntry[NUM_VALID_RATINGS];

	train_sizeA = fillWithData(train_dataA, mu_train, 1, 3);
	valid_sizeA = fillWithData(valid_dataA, mu_train, 2);
	probe_sizeA = fillWithData(probe_dataA, mu_probe);
	

	// Test predicts

	printf("Temporary size is %d\n", train_sizeA);
	timerStart(2);
	printf("Valid RMSE:	         %.4f\n", rmse = computeRMSE(mod_kNN, 
												u, 
												mat_startingPointsUM_R, 
												mat_startingPointsUM_N,
												mat_startingPointsMatArray, 
												train_dataA, 
												valid_dataA, 
												valid_sizeA));

	/*printf("Valid RMSE PARALLEL: %.4f\n", rmse = computeRmseParallel(
												mod_kNN,
												u,
												mat_startingPointsUM_R,
												mat_startingPointsUM_N,
												mat_startingPointsMatArray, 
												train_dataA,
												valid_dataA,
												valid_sizeA));
*/

	printf("Computing valid RMSE takes %.4f\n", timerGetS(2));


/*
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);*/

	printf("Probe RMSE:      %.4f\n", computeRMSE(mod_kNN, 
												u, 
												mat_startingPointsUM_R, 
												mat_startingPointsUM_N,
												mat_startingPointsMatArray, 
												train_dataA, 
												probe_dataA, 
												probe_sizeA));	

	////////////////
	// Training loop
	////////////////
	
	timerStart(2);
	for (int i = 0; i < Niter; i++)
	{
		printf("\n\n#[%3d]  learning rate %.7f\n", i + 1, l);

		/*printf("-----------Sanity check ratings---------------------\n");

		for (int uu = 0; uu < 3; uu++){
		int m;
		for (int i = 0; i < u->nMovRated_R[uu]; i++){
		m = u->idMovRated_R[i];
		printf("r[%u, %u] = %.8f \n", uu, m, MEAN_RATING + predict(mod_kNN, u, mat_startingPointsUM_R, mat_startingPointsUM_N, mat_startingPointsMatArray, uu, m, train_dataA));
		}
		}
		*/
		/*printf("W = ");
		for (int ii = 0; ii < 5; ii++) printf("%.7f, ", mod_kNN->m_corr_W[ii]) ;
		printf("\n\n");

		printf("C = ");
		for (int ii = 0; ii < 5; ii++) printf("%.7f, ", mod_kNN->m_baseline_C[ii]);
		printf("\n\n");

		printf("bu = ");
		for (int ii = 0; ii < 5; ii++) printf("%.5f, ", mod_kNN->v_bu[ii]);
		printf("\n\n");

		printf("bm = ");
		for (int ii = 0; ii < 5; ii++) printf("%.5f, ", mod_kNN->v_bm[ii]);
		printf("\n\n");*/

		/*if (i % 10 == 0){
			std::cout << "Press ENTER to continue....." << std::endl << std::endl;
			std::cin.ignore(1);
			}*/

		// Iteration
		if (!smartIteration(mod_kNN, mod_kNN_temp, l, rmse))	break;

		fprintf(f, "%d, %.7f, %.7f\n", i + 1, rmse,
			probermse = computeRMSE(mod_kNN,
			u,
			mat_startingPointsUM_R,
			mat_startingPointsUM_N,
			mat_startingPointsMatArray,
			train_dataA,
			probe_dataA,
			probe_sizeA));

		// Clever adjusting learning rate
		// keepBestProbeRMSE(probermse);
		// if ((i+1) % TUNE_FREQ == 0) ChooseRate(l);

		if ((i + 1) % 5 == 0) {
			printf("---Probe RMSE:	    == %.4f ==\n", probermse);
		}
		
			printf("---Estimate time left: %.3f m", (Niter - i - 1)*(timerGetS(2) / (i + 1)) / 60);
	}

	// Loop converged or we hit max iters

	fclose(f);
	printf("\n\n------------------------\n\n");
	printf("averate iteration time: %.3f\n", timerGetS(2) / Niter);


	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	// Write mod_kNN to file
	double validRMSE = computeRMSE(mod_kNN,
							u,
							mat_startingPointsUM_R,
							mat_startingPointsUM_N,
							mat_startingPointsMatArray,
							train_dataA,
							// train_sizeA,
							valid_dataA,
							valid_sizeA);


	double probeRMSE = computeRMSE(mod_kNN,
							u,
							mat_startingPointsUM_R,
							mat_startingPointsUM_N,
							mat_startingPointsMatArray,
							train_dataA,
							// train_sizeA,
							probe_dataA,
							probe_sizeA);

	char mbstr[100];
	std::strftime(mbstr, sizeof(mbstr), "%d-%m-%H%M", std::localtime(&t));

	std::string str1 = std::to_string(validRMSE);
	std::string str2 = std::to_string(probeRMSE);
	strcat(mbstr, str1.c_str());
	strcat(mbstr, str2.c_str());
	strcat(mbstr, "all.soln");

	save_kNN(mod_kNN, mbstr);
	

	std::cout << "FIN -- Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	//	saveSVD2(bestSVD, "22-04-0001all-best.svd2");
	
	delete[] mat_startingPointsMatArray;
	delete[] mat_startingPointsUM_R;
	delete u;
	delete[] train_dataA;
	delete[] train_tempA;
	delete[] probe_dataA;
	delete[] valid_dataA;

	_CrtDumpMemoryLeaks();


}