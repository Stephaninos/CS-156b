#include <stdio.h>
#include <iostream>
#include "types.h"
#include "timer.h"


#include <ctime>
#include <cstring>
#include <sstream>


// USE with UM-file!! Only needed once to generate file with counting stats and ids
int countStatsAndWriteToFile(
	char * mu_all, char * out_file){
		
	timerStart(0);
	FILE * f = fopen(mu_all, "rb");
	/*FILE * g = fopen(mu_probe, "rb");
	FILE * h = fopen(mu_test, "rb");*/
	
	FileEntry c;
	FileEntry cprobe;
	FileEntry ctest;	
	UserStats *u = new UserStats;
		
	unsigned short zeros[USER_NUM] = {};
	memcpy((*u).nMovSeen_N, zeros, sizeof(short) * USER_NUM);
	memcpy((*u).nMovRated_R, zeros, sizeof(short)* USER_NUM);
	
	int i = 0;
	int j = 0;
	int seenendofuser1 = 0;

	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;
			
		// To not take the blank line at the end into account
		if (seenendofuser1 == 1 && c.user == 1) break;
		if (c.user == 2){				
			seenendofuser1 = 1;
		};

		(*u).idMovSeen_N[i] = c.movie - 1;
		(*u).nMovSeen_N[c.user - 1] += 1;
				
		if (c.category == 1 || c.category == 3){
			(*u).idMovRated_R[j] = c.movie - 1;
			(*u).nMovRated_R[c.user - 1] += 1;
			j++;
		};

		if (i % 1000000 == 0) printf("%.1f million lines done \n", i / 1000000.);

		i++;
		
	};

	fclose(f);
	
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);
	
	// Blabla
	
	for (int j = 0; j < 280; j++) printf("N: User saw movie: %u \n", (*u).idMovSeen_N[j]);
	
	// printf("Last entry: %u", (*u).idMovSeen_N[TOTAL_NUM_RATINGS-1]);
	// for (int j = 0; j < 100; j++) printf("N: User %u saw %u movies \n", j, (*u).nMovSeen_N[j]);
	
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	/*for (int j = 0; j < 200; j++) printf("R: User .  movie: %u \n", (*u).idMovRated_R[j]);
	for (int j = 0; j < 100; j++) printf("R: User %u saw %u movies \n", j, (*u).nMovRated_R[j]);
		
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);*/
	
	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
	
	// Write stuff
	
	printf("Now writing to file...\n");
	
	FILE * m = fopen(out_file, "wb");
	fwrite(u, sizeof(UserStats), 1, m);
	fclose(m);
	
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);
	
	return 1;
}

bool fillUserStats(char * filepath, UserStats *u){
	FILE * m = fopen(filepath, "rb");

	unsigned short c;
	for (int i = 0; i < USER_NUM; i++){
		fread(&c, 2, 1, m);
		(*u).nMovSeen_N[i] = c;
		if (c > 0){
			(*u).invSqRootSizeN[i] = 1 / pow(c, 0.5);
		}
		else{
			(*u).invSqRootSizeN[i] = 1;
		}
	}

	fseek(m, sizeof(unsigned short) + USER_NUM*sizeof(float), SEEK_CUR);

	for (int i = 0; i < TOTAL_NUM_RATINGS; i++){
		fread(&c, 2, 1, m);
		(*u).idMovSeen_N[i] = c;		
	};
	for (int i = 0; i < USER_NUM; i++){
		fread(&c, 2, 1, m);
		(*u).nMovRated_R[i] = c;
		if (c>0){
			(*u).invSqRootSizeR[i] = 1 / pow(c, 0.5);
		}
		else{
			(*u).invSqRootSizeR[i] = 1;
		}
	};

	fseek(m, sizeof(unsigned short) + USER_NUM*sizeof(float), SEEK_CUR);

	for (int i = 0; i < NUM_TRAIN_RATINGS; i++){
		fread(&c, 2, 1, m);
		(*u).idMovRated_R[i] = c;	
	};	
	fclose(m);
	return true;
}


//int gatherUserRatedRatings(DataEntry* list, char* file, int size)
//{
//	timerStart(0);
//	FILE * f = fopen(file, "rb");
//	FileEntry c;
//	double total = 0;
//	int i = 0;
//	while (true)
//	{
//		fread(&c, 10, 1, f);
//		if (feof(f) != 0) break;
//		list[i].movie_ptr = c.movie - 1;
//		list[i].user_ptr = c.user - 1;
//		list[i].rating = c.rating - MEAN_RATING;
//		total += c.rating;
//		i++;
//	}
//	fclose(f);
//
//	printf("\n");
//	printf("Read from file %s\n", file);
//	printf("Found %d entries\n", i);
//	printf("Mean rating over the file is %.5lf\n", total / i);
//	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
//	printf("\n");
//	return i;
//}
//
//
//
//static inline float predict(svd_entry * users, svd_entry * movies, unsigned int a, unsigned int b)
//{
//	float s = 0;
//	for (int i = 0; i < SVD_dim; i++)
//	{
//		s += users[a].params[i] * movies[b].params[i];
//	}
//	return s;
//}
//
//void dumpPred(svd_entry * users, svd_entry * movies, DataEntry* testset, int size, char* filename)
//{
//	FILE * f = fopen(filename, "wt");
//	double p;
//	for (int i = 0; i < size; i++)
//	{
//		p = MEAN_RATING+predict(users, movies, testset[i].user_ptr, testset[i].movie_ptr);
//		if (p > 5)p = 5;
//		if (p < 1)p = 1;
//		fprintf(f, "%.5f\n", p);
//	}
//	fclose(f);
//}


int fillWithData(DataEntry* list, char* file)
{
	timerStart(0);
	FILE * f = fopen(file, "rb");
	FileEntry c;
	double total=0;
	int i = 0;
	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;
		list[i].movie_ptr = c.movie - 1;
		list[i].user_ptr = c.user - 1;
		list[i].rating = c.rating - MEAN_RATING;
		total += c.rating;
		i++;
	}
	fclose(f);

	printf("\n");
	printf("Read from file %s\n", file);
	printf("Found %d entries\n", i);
	printf("Mean rating over the file is %.5lf\n", total/i);
	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
	printf("\n");
	return i;
}

int fillWithData(DataEntry* list, char* file, int type)
{
	timerStart(0);
	FILE * f = fopen(file, "rb");
	FileEntry c;
	double total=0;
	int i = 0;
	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;
		if (c.category != type) continue;
		list[i].movie_ptr = c.movie - 1;
		list[i].user_ptr = c.user - 1;
		list[i].rating = c.rating - MEAN_RATING;
		total += c.rating;
		i++;
	}
	fclose(f);

	printf("\n");
	printf("Read from file %s\n", file);
	printf("Found %d entries\n", i);
	printf("Mean rating over the file is %.5lf\n", total / i);
	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
	printf("\n");
	return i;
}


int fillWithData(DataEntry* list, char* file, int type1, int type2)
{
	timerStart(0);
	FILE * f = fopen(file, "rb");
	FileEntry c;
	double total = 0;
	int i = 0;
	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;
		if (c.category != type1 && c.category != type2 ) continue;
		list[i].movie_ptr = c.movie - 1;
		list[i].user_ptr = c.user - 1;
		list[i].rating = c.rating - MEAN_RATING;
		total += c.rating;
		i++;
	}
	fclose(f);

	printf("\n");
	printf("Read from file %s\n", file);
	printf("Found %d entries\n", i);
	printf("Mean rating over the file is %.5lf\n", total / i);
	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
	printf("\n");
	return i;
}

void save_kNN(Model * kNN, char* filename)
{
	FILE *f = fopen(filename, "wb");
	fwrite(kNN, sizeof(Model), 1, f);
	fclose(f);
}

void read_kNN(Model * kNN, char * filename)
{
	FILE *f = fopen(filename, "rb");
	fread(kNN, sizeof(Model), 1, f);
	fclose(f);
}

void read_biases(Biases * kNN, char * filename)
{
	FILE *f = fopen(filename, "rb");
	fread(kNN, sizeof(Biases), 1, f);
	fclose(f);
}

void saveParam(time_t * time)
{
	char name[1024];
	std::strftime(name, sizeof(name), "%d-%m-%H%M", std::localtime(time));
	strcat(name, RUN_NAME);
	strcat(name, ".param");
	FILE * f = fopen(name, "wt");
	fprintf(f, "%s\n", RUN_COMMENT);	
	fprintf(f, "REGULARIZATION_CONST_W    :  %.8f\n", REGULARIZATION_CONST_W);
	fprintf(f, "REGULARIZATION_CONST_C    :  %.8f\n", REGULARIZATION_CONST_C);
	fprintf(f, "SEEDRANGE_BIAS    :  %.8f\n", SEEDRANGE_BIAS);
	fprintf(f, "SEEDRANGE_CORR    :  %.8f\n", SEEDRANGE_CORR);
	fprintf(f, "SEEDRANGE_BASELINE    :  %.8f\n", SEEDRANGE_BASELINE);
	fprintf(f, "LEARNING_DEC :  %.8f\n", LEARNING_DEC);
	fprintf(f, "LEARNING_INC :  %.8f\n", LEARNING_INC);
	fprintf(f, "LEARNING_DWN :  %.8f\n", LEARNING_DOWN);
	fprintf(f, "LEARNING_UP  :  %.8f\n", LEARNING_UP);
	fprintf(f, "TUNE_FREQ    :  %d\n", TUNE_FREQ);
	fclose(f);
}











//
//
//
//// If there are entries in probe for user #c.user, we add all the entries in probe (and test)
//while (currentIDprobe != c.user)
//{
//	fread(&cprobe, 10, 1, g);
//	if (feof(g) != 0) break;
//
//	// If there are entries in test for user #cprobe.user, we add all the entries in test
//	while (currentIDtest != cprobe.user)
//	{
//		fread(&ctest, 10, 1, h);
//		if (feof(h) != 0) break;
//
//		if (ctest.user != cprobe.user){
//			fseek(h, -10, SEEK_CUR);
//			break;
//		};
//
//		(*u).idMovSeen_N[i] = c.movie - 1;
//		(*u).nMovSeen_N[c.user - 1] += 1;
//		i++;
//		;
//	}
//
//	if (cprobe.user != c.user){
//		fseek(g, -10, SEEK_CUR);
//		break;
//	};
//
//	(*u).idMovSeen_N[i] = c.movie - 1;
//	(*u).nMovSeen_N[c.user - 1] += 1;
//	i++;
//}