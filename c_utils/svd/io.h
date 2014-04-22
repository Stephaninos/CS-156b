#pragma once
#include <vector>
#include "types.h"

int fillWithData(std::vector<data_entry>& list, char* file, int size);
int fillWithDataA(data_entry* list, char* file, int size);
int fillWithDataA(data_entry* list, char* file, int size, int type);
void saveSVD(svd_entry*, int, char*);
void readSVD(svd_entry*, int, char*);
void readSVD2(model*, char*);
void saveSVD2(model*, char*);
void saveParam();
void dumpPred(svd_entry * users, svd_entry * movies, data_entry* testset, int size, char* filename);

//void buildUserList(char * um);
//void buildMovieList(char * um);
//int getAddr(unsigned int id, std::vector<unsigned int> &list);