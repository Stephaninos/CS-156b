#include "../svd/types.h"
#include "../svd/io.h"
#include "../svd/train.h"

char * mu_test = "../../mu/all.dta.test";
char * mu_probe = "../../mu/all.dta.probe";
char * user_svd = "../svd/user.svd2";
char * movie_svd = "movie9587[run7].svd";
char * test_out = "test7.csv";
char * probe_out = "probe7.csv";
char * probe_rate = "probe_ratings.csv";


data_entry* probe_dataA;
int probe_sizeA;

data_entry* test_dataA;
int test_sizeA;

model SVD;


int main()
{
	probe_dataA = new data_entry[2000000];
	test_dataA = new data_entry[3000000];
	probe_sizeA = fillWithDataA(probe_dataA, mu_probe, 2000000);
	test_sizeA = fillWithDataA(test_dataA, mu_test, 3000000);

	readSVD2(&SVD, user_svd);

	printf("Probe RMSE:%.6f\n", RMSEA(&SVD, probe_dataA, probe_sizeA));

	//dumpPred(userSVD, movieSVD, test_dataA, test_sizeA, test_out);
	//dumpPred(userSVD, movieSVD, probe_dataA, probe_sizeA, probe_out);

	//FILE * f = fopen(probe_rate, "wt");
	//for (int i = 0; i < probe_sizeA; i++)
	//{
	//	fprintf(f, "%.5f\n", probe_dataA[i].rating+MEAN_RATING);
	//}
	//fclose(f);
}