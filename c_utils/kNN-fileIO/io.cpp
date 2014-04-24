#include <stdio.h>
#include <iostream>
#include "types.h"
#include "timer.h"

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
	memcpy((*u).allRatedMoviesByUserSize_N, zeros, sizeof(short) * USER_NUM);
	memcpy((*u).allRatedMoviesWithRatingByUserSize_R, zeros, sizeof(short)* USER_NUM);
	
	int i = 0;
	int j = 0;
			
	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;				

		(*u).allRatedMoviesByUser_N[i]				= c.movie - 1;
		(*u).allRatedMoviesByUserSize_N[c.user - 1] += 1;
		
		// if (c.user == 1) printf("N[%u] = %u \n", c.user - 1 + (*u).allRatedMoviesByUserSize_N[c.user - 1], (*u).allRatedMoviesByUser_N[c.user - 1 + (*u).allRatedMoviesByUserSize_N[c.user - 1]]);
		if (c.user == 1) printf("Count is: %u \n", (*u).allRatedMoviesByUserSize_N[c.user - 1]);

		if (c.category == 1 || c.category == 3){
			(*u).allRatedMoviesWithRatingByUser_R[j] = c.movie - 1;
			(*u).allRatedMoviesWithRatingByUserSize_R[c.user - 1] += 1;
			j++;
		};

		if (i % 1000000 == 0) printf("%.1f perc done \n", i/1000000.);

		i++;		
		
	};

	fclose(f);
	//fclose(g);
	//fclose(h);
	
	for (int j = 0; j < 200; j++) printf("User 1 saw movie: %u \n", (*u).allRatedMoviesByUser_N[j]);
	for (int j = 0; j < 100; j++) printf("User %u saw %u movies \n", j, (*u).allRatedMoviesByUserSize_N[j]);

	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	for (int j = 0; j < 200; j++) printf("User 1 saw movie: %u \n", (*u).allRatedMoviesWithRatingByUser_R[j]);
	for (int j = 0; j < 100; j++) printf("User %u saw %u movies \n", j, (*u).allRatedMoviesWithRatingByUserSize_R[j]);
		
	std::cout << "Press ENTER to continue....." << std::endl << std::endl;
	std::cin.ignore(1);

	printf("Reading took %.3f s\n", float(timerGet(0)) / 1000);
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
		(*u).allRatedMoviesByUserSize_N[i] = c;
		(*u).invSqRootSizeN[i]			   = 1 / pow(c, 0.5);
	};
	for (int i = 0; i < TOTAL_NUM_RATINGS; i++){
		fread(&c, 2, 1, m);
		(*u).allRatedMoviesByUser_N[i] = c;
	};
	for (int i = 0; i < USER_NUM; i++){
		fread(&c, 2, 1, m);
		(*u).allRatedMoviesWithRatingByUserSize_R[i] = c;
		(*u).invSqRootSizeR[i]						 = 1 / pow(c, 0.5);
		// if (i<100) printf("Read: %u \n", c);
	};
	for (int i = 0; i < NUM_TRAIN_RATINGS; i++){
		fread(&c, 2, 1, m);
		(*u).allRatedMoviesWithRatingByUser_R[i] = c;
		// if (i<100) printf("User %u rated movie %u \n", i, c);
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


int fillWithData(DataEntry* list, char* file, int size)
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

int fillWithData(DataEntry* list, char* file, int size, int type)
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


void saveParam(char * time)
{
	char name[1024];
	strcpy(name, time);
	strcat(name, RUN_NAME);
	strcat(name, ".param");
	FILE * f = fopen(name, "wt");
	fprintf(f, "%s\n", RUN_COMMENT);
	fprintf(f, "SVD_dim      :  %d\n", SVD_dim);
	fprintf(f, "REGULAR_U    :  %.8f\n", REGULARIZATION_CONST_U);
	fprintf(f, "REGULAR_M    :  %.8f\n", REGULARIZATION_CONST_M);
	fprintf(f, "SEEDRANGE    :  %.8f\n", SEEDRANGE);
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
//		(*u).allRatedMoviesByUser_N[i] = c.movie - 1;
//		(*u).allRatedMoviesByUserSize_N[c.user - 1] += 1;
//		i++;
//		;
//	}
//
//	if (cprobe.user != c.user){
//		fseek(g, -10, SEEK_CUR);
//		break;
//	};
//
//	(*u).allRatedMoviesByUser_N[i] = c.movie - 1;
//	(*u).allRatedMoviesByUserSize_N[c.user - 1] += 1;
//	i++;
//}