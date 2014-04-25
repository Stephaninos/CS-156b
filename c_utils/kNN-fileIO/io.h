#pragma once
#include <vector>
#include "types.h"

int countStatsAndWriteToFile(
	char * mu_all, char * out_file);

bool fillUserStats(char * filepath, UserStats *u);
void saveParam(time_t * time);

int fillWithData(std::vector<DataEntry>& list, char* file, int size);
int fillWithData(DataEntry* list, char* file);
int fillWithData(DataEntry* list, char* file, int type);
int fillWithData(DataEntry* list, char* file, int type1, int type2);

void save_kNN(Model*, char*);
void read_kNN(Model*, char*);

//void dumpPred(svd_entry * users, svd_entry * movies, DataEntry* testset, int size, char* filename);
//
////void buildUserList(char * um);
////void buildMovieList(char * um);
////int getAddr(unsigned int id, std::vector<unsigned int> &list);