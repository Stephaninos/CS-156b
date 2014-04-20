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

int fillWithDataA(data_entry* list, char* file, int size, int type)
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

void saveSVD(svd_entry* data, int size, char* filename)
{
	FILE *f = fopen(filename, "wb");
	fwrite(data, size*sizeof(svd_entry),1, f);
	fclose(f);
}


void saveParam()
{
	char name[1024];
	strcpy(name, RUN_NAME);
	strcat(name, ".param");
	FILE * f = fopen(name,"wt");
	fprintf(f, "%s\n", RUN_COMMENT);
	fprintf(f, "SVD_dim      :  %d\n", SVD_dim);
	fprintf(f, "REGULAR_U    :  %.8f\n", REGULARIZATION_CONST_U);
	fprintf(f, "REGULAR_M    :  %.8f\n", REGULARIZATION_CONST_M);
	fprintf(f, "LEARNING_DEC :  %.8f\n", LEARNING_DEC);
	fprintf(f, "LEARNING_INC :  %.8f\n", LEARNING_INC);
	fprintf(f, "LEARNING_DWN :  %.8f\n", LEARNING_DOWN);
	fprintf(f, "LEARNING_UP  :  %.8f\n", LEARNING_UP);
	fprintf(f, "TUNE_FREQ    :  %d\n", TUNE_FREQ);
	fclose(f);
}