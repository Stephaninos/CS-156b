#include <stdio.h>
#include <vector>
#include "types.h"
#include "timer.h"

int fillWithData(std::vector<data_entry>& list, char* file, int size)
{
	timerStart(0);
	FILE * f = fopen(file, "rb");
	file_entry c;
	int i = 0;
	list.clear();
	list.resize(size);
	while (true)
	{
		fread(&c, 10, 1, f);
		if (feof(f) != 0) break;
		list[i].movie_ptr = c.movie - 1;
		list[i].user_ptr = c.user - 1;
		list[i].rating = c.rating-MEAN_RATING;
		i++;
	}
	list.resize(i);
	fclose(f);
	
	printf("\n");
	printf("Read from file %s\n", file);
	printf("Found %d entries\n", i);
	printf("Reading took %.3f s", float(timerGet(0)) / 1000);
	printf("\n");
	return i;
}

int fillWithDataA(data_entry* list, char* file, int size)
{
	timerStart(0);
	FILE * f = fopen(file, "rb");
	file_entry c;
	double total;
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
//std::vector<unsigned int> userIdList;
//std::vector<unsigned int> movieIdList;
//
//int getAddr(unsigned int id, std::vector<unsigned int> &list)
//{
//	int left = 0,right = list.size()-1;
//	while (right > left)
//	{
//		int attempt = (left + right) / 2;
//		if (list[attempt] > id)
//		{
//			right = attempt - 1;
//		}
//		else
//		{
//			if (list[attempt] < id)
//			{
//				left = attempt + 1;
//			}
//			else
//			{
//				left = right = attempt;
//			}
//		}
//	}
//	if (list[left] == id) return left;
//	return -1;
//}
//
//void buildUserList(char * um)
//{
//	FILE * f = fopen(um,"rb");
//	file_entry c;
//	userIdList.clear();
//	while (true)
//	{
//		fread(&c, 10, 1, f);
//		if (feof(f) != 0) break;
//		if(userIdList.empty())userIdList.push_back(c.user);
//		else if (userIdList[userIdList.size() - 1] < c.user) userIdList.push_back(c.user);
//	}
//	fclose(f);
//	printf("Built user list; # of users is %d\n", userIdList.size());
//	printf("last id is %u\n", userIdList[userIdList.size() - 1]);
//}
//
//void buildMovieList(char * mu)
//{
//	FILE * f = fopen(mu, "rb");
//	file_entry c;
//	movieIdList.clear();
//	while (true)
//	{
//		fread(&c, 10, 1, f);
//		if (feof(f) != 0) break;
//		if (movieIdList.empty())movieIdList.push_back(c.movie);
//		else if (movieIdList[movieIdList.size() - 1] < c.movie) movieIdList.push_back(c.movie);
//	}
//	fclose(f);
//	printf("Built movie list; # of movies is %d\n", movieIdList.size());
//	printf("last id is %u\n", movieIdList[movieIdList.size() - 1]);
//}