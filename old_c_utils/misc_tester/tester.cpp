#include <stdio.h>

struct train_point
{
	unsigned int user;
	unsigned short movie;
	unsigned short date;
	unsigned char rating;
	unsigned char category;
};

int main()
{
	FILE *f = fopen("test2.dta.train","rb");
	train_point trpoint;
	printf("train:\n");
	while (feof(f) == 0)
	{
		fread(&trpoint, 10, 1, f);
		if (feof(f)) break;
		printf("%u %hu %hu %hhu %hhu\n", trpoint.user, trpoint.movie, trpoint.date, trpoint.rating, trpoint.category);
	}
	fclose(f);
	f = fopen("test2.dta.probe", "rb");
	printf("probe:\n");
	while (feof(f) == 0)
	{
		fread(&trpoint, 10, 1, f);
		if (feof(f)) break;
		printf("%u %hu %hu %hhu %hhu\n", trpoint.user, trpoint.movie, trpoint.date, trpoint.rating, trpoint.category);
	}
	fclose(f);
	f = fopen("test2.dta.test", "rb");
	printf("test:\n");
	while (feof(f) == 0)
	{
		fread(&trpoint, 10, 1, f);
		if (feof(f)) break;
		printf("%u %hu %hu %hhu %hhu\n", trpoint.user, trpoint.movie, trpoint.date, trpoint.rating, trpoint.category);
	}
	fclose(f);
}