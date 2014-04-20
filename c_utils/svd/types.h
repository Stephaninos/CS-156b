#pragma once

#define SVD_dim 40
#define USER_NUM 458293
#define MOVIE_NUM 17770

#define MEAN_RATING 3.60861
#define THRESHOLD 0.0001

struct file_entry
{
	unsigned int user;
	unsigned short movie;
	unsigned short date;
	unsigned char rating;
	unsigned char category;
};

struct data_entry
{
	float rating;
	unsigned int user_ptr;
	unsigned int movie_ptr;
};

struct svd_entry
{
	float params[SVD_dim];
};