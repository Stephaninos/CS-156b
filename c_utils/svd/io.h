#pragma once
#include <vector>
#include "types.h"

int fillWithData(std::vector<data_entry>& list, char* file, int size);
int fillWithDataA(data_entry* list, char* file, int size);
int fillWithDataA(data_entry* list, char* file, int size, int type);
void saveSVD(svd_entry*, int, char*);

//void buildUserList(char * um);
//void buildMovieList(char * um);
//int getAddr(unsigned int id, std::vector<unsigned int> &list);