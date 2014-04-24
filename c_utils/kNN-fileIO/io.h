#pragma once
#include <vector>
#include "types.h"

int countStatsAndWriteToFile(
	char * mu_all, char * out_file);

bool fillUserStats(char * filepath, UserStats *u);
void saveParam(char * time);

int fillWithData(std::vector<DataEntry>& list, char* file, int size);
int fillWithData(DataEntry* list, char* file, int size);
int fillWithData(DataEntry* list, char* file, int size, int type);

void save_kNN(Model*, char*);
void read_kNN(Model*, char*);

//void dumpPred(svd_entry * users, svd_entry * movies, DataEntry* testset, int size, char* filename);
//
////void buildUserList(char * um);
////void buildMovieList(char * um);
////int getAddr(unsigned int id, std::vector<unsigned int> &list);