#include <stdio.h>
#include <vector>
#include <algorithm>

char * dataset = "all.dta.train";
char * outfile = "moviesummary.csv";

struct data_entry
{
	unsigned int user;
	unsigned short movie;
	unsigned short date;
	unsigned char rating;
	unsigned char category;
};

struct rating_event
{
	unsigned short date;
	unsigned char rating;
};

bool rating_comp(rating_event a, rating_event b)
{
	return a.date < b.date;
}

struct movie_summary
{
	unsigned short movie;
	unsigned int ratings;
	float rating_mean;
	float rating_var;
	unsigned short first_rating;
	unsigned short last_rating;

	unsigned short viewdates[10];
	float ratingdates[10];

};// "movie, ratings, rating_mean, rating_var, first_rating, last_rating, week_ratings, month_ratings, hyear_ratings, year_ratings, syear_ratings, date_hratings, week_rating, month_rating, hyear_rating, year_rating, syear_rating\n"

void fprint_movie(FILE* f, movie_summary sum)
{
	fprintf(f,"%u, %u, %.3f, %.3f, %hu, %hu",
		sum.movie,sum.ratings,sum.rating_mean,sum.rating_var,sum.first_rating,sum.last_rating);
	for (int i = 0; i < 10; i++)
		fprintf(f, ", %hu", sum.viewdates[i]);
	for (int i = 0; i < 10; i++)
		fprintf(f, ", %.3f", sum.ratingdates[i]);
	fprintf(f, "\n");
}

int main()
{
	FILE *fdata = fopen(dataset, "rb");
	FILE *fout = fopen(outfile, "wt");

	fprintf(fout, "movie, ratings, rating_mean, rating_var, first_rating, last_rating");
	for (int i = 0; i < 10; i++)
		fprintf(fout, ", viewdate%d", i);
	for (int i = 0; i < 10; i++)
		fprintf(fout, ", ratingdate%d", i);
	fprintf(fout, "\n");

	std::vector<rating_event> ratings;
	data_entry cmovie;
	data_entry entry;
	movie_summary sum;
	int i = 0;
	unsigned int rtotal;
	unsigned int r2total;
	fread(&cmovie, 10, 1, fdata);
	entry = cmovie;
	while (true)
	{
		sum.movie = cmovie.movie;
		sum.ratings = 1;
		rtotal = cmovie.rating;
		r2total = cmovie.rating*cmovie.rating;
		sum.first_rating = cmovie.date;
		sum.last_rating = cmovie.date;

		ratings.push_back({ cmovie.date, cmovie.rating });
		if (feof(fdata)) break;
		while (true)
		{
			fread(&entry, 10, 1, fdata);
			if (feof(fdata) || entry.movie != cmovie.movie)
			{
				cmovie = entry;
				break;
			}
			ratings.push_back({ entry.date, entry.rating });
			rtotal += entry.rating;
			r2total += entry.rating*entry.rating;
			sum.ratings++;
			if (sum.first_rating > entry.date) sum.first_rating = entry.date;
			if (sum.last_rating < entry.date) sum.last_rating = entry.date;
		}
		/* ANALYZE HERE */

		sum.rating_mean = double(rtotal) / sum.ratings;
		sum.rating_var = double(r2total) / sum.ratings - sum.rating_mean*sum.rating_mean;

		std::sort(ratings.begin(), ratings.end(), rating_comp);
		unsigned int rate= 0;
		for (int i = 1; i <= 10; i++)
		{
			sum.viewdates[i - 1] = ratings[(sum.ratings*i)>10? (sum.ratings*i) / 10 - 1:0].date - sum.first_rating;
		}

		for (int i = 0; i < 10; i++)
		{
			rate = 0;
			for (int d = (sum.ratings*i) / 10; d < (sum.ratings*(i + 1)) / 10; d++)
			{
				rate += ratings[d].rating;
			}
			sum.ratingdates[i] = double(rate) / sum.ratings * 10;
		}
		
		fprint_movie(fout, sum);

		printf("done movie %d; %d\n", sum.movie, sum.ratings-ratings.size());

		ratings.clear();
	}

	fclose(fout);
	fclose(fdata);
	return 0;
}