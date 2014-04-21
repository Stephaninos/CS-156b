#undef UNICODE
#include <stdio.h>
#include <string.h>

int main(int argc, char ** argv)
{
	if (argc == 1)
	{
		printf("Usage: text_to_binary dataname1 indexname1 [dataname 2 ...]\nTranslated files will be dataname1.train, dataname1.test, dataname1.probe ...\n");
		return 0;
	}
	if ((argc - 1) % 2 != 0)
	{
		printf("*error* Number of argumenst must be even\n");
		printf("Usage: text_to_binary dataname1 indexname1 [dataname 2 ...]\nTranslated files will be dataname1.train, dataname1.test, dataname1.probe ...\n");
		return 0;
	}
	for (int i = 1; i <= (argc-1)/2; i++)
	{
		printf("working on file %d...\n", i);
		FILE *fin = fopen(argv[2 * i - 1], "rt");
		FILE *find = fopen(argv[2 * i], "rt");
		if (fin == 0)
		{
			printf("*error* couldn't read from %s\n", argv[2 * i - 1]);
		}
		if (find == 0)
		{
			printf("*error* couldn't read from %s\n", argv[2 * i]);
		}
		char outname[1024];
		strcpy(outname, argv[2*i-1]);
		strcat(outname, ".train");
		FILE *ftrain = fopen(outname, "wb");
		if (ftrain == 0)
		{
			printf("*error* couldn't write to %s\n", outname);
		}
		strcpy(outname, argv[2 * i - 1]);
		strcat(outname, ".test");
		FILE *ftest = fopen(outname, "wb");
		if (ftest == 0)
		{
			printf("*error* couldn't write to %s\n", outname);
		}
		strcpy(outname, argv[2 * i - 1]);
		strcat(outname, ".probe");
		FILE *fprobe = fopen(outname, "wb");
		if (fprobe == 0)
		{
			printf("*error* couldn't write to %s\n", outname);
		}

		unsigned int user;
		unsigned short movie;
		unsigned short date;
		unsigned short rating;
		unsigned short category = 1;
		unsigned char crating;
		unsigned char ccategory = 1;
		unsigned int stats[6];
		memset(stats, 0, 6 * sizeof(int));
		unsigned int N = 0;

		while (feof(fin) == 0)
		{
			fgets(outname, 1024, fin);
			sscanf(outname, "%u %hu %hu %hu", &user, &movie, &date, &rating);
			if (strlen(outname)<2) break;
			if (category<6&&feof(find))
			{
				printf("*warning* end of index file, assuming all further points to be training!\n");
				category = 6;
			}
			else if (category < 6)
			{
				fgets(outname, 1024, find);
				sscanf(outname, "%hu", &category);
			}

			ccategory = category;
			crating = rating;
			stats[category - 1]++;
			if (category < 4 || category == 6)
			{
				fwrite(&user, sizeof(user), 1, ftrain);
				fwrite(&movie, sizeof(movie), 1, ftrain);
				fwrite(&date, sizeof(date), 1, ftrain);
				fwrite(&crating, sizeof(crating), 1, ftrain);
				fwrite(&ccategory, sizeof(ccategory), 1, ftrain);
			}
			if (category == 4)
			{
				fwrite(&user, sizeof(user), 1, fprobe);
				fwrite(&movie, sizeof(movie), 1, fprobe);
				fwrite(&date, sizeof(date), 1, fprobe);
				fwrite(&crating, sizeof(crating), 1, fprobe);
				fwrite(&ccategory, sizeof(ccategory), 1, fprobe);
			}
			if (category == 5)
			{
				fwrite(&user, sizeof(user), 1, ftest);
				fwrite(&movie, sizeof(movie), 1, ftest);
				fwrite(&date, sizeof(date), 1, ftest);
				fwrite(&crating, sizeof(crating), 1, ftest);
				fwrite(&ccategory, sizeof(ccategory), 1, ftest);
			}
			N++;
			if (N % 1000000 == 0)
			{
				printf("%u lines processed\n", N);
			}
		}

		fclose(fin);
		fclose(find);
		fclose(ftrain);
		fclose(ftest);
		fclose(fprobe);

		FILE* rep = fopen("t2b_report.txt", "at");
		fprintf(rep, "***********\n");
		fprintf(rep, "processed files\n");
		fprintf(rep, "%s\n", argv[2 * i - 1]);
		fprintf(rep, "%s\n", argv[2 * i]);
		fprintf(rep, "found %u entries, out of which\n", N);
		fprintf(rep, "train 1 : %u\n", stats[0]);
		fprintf(rep, "train 2 : %u\n", stats[1]);
		fprintf(rep, "train 3 : %u\n", stats[2]);
		fprintf(rep, "train 6 : %u\n", stats[5]);
		fprintf(rep, "probe   : %u\n", stats[3]);
		fprintf(rep, "test    : %u\n", stats[4]);
		fclose(rep);
	}
	
	return 0;
}