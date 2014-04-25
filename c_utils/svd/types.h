#pragma once

#define SVD_dim 150
#define USER_NUM 458293
#define MOVIE_NUM 17770

#define MEAN_RATING 3.60861
#define THRESHOLD 0
#define NUM_THREADS 8

#define REGULARIZATION_CONST_U 0.002f
#define REGULARIZATION_CONST_M 0.002f

#define LEARNING_RATE 0.0001
#define B_LEARNING_RATIO 0.02
#define LEARNING_DEC 0.8
#define LEARNING_INC 1.005

#define LEARNING_UP 1.5
#define LEARNING_DOWN 1.5
#define TUNE_FREQ 300000

#define INIT_RANGE 0.05

#define ITERATION_NUM 1500

#define RUN_NAME "22-4-0001run1"

#define RUN_COMMENT "random init [0,0.1]"

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

struct model
{
	svd_entry userSVD[USER_NUM];
	svd_entry movieSVD[MOVIE_NUM];
	float bu[USER_NUM];
	float bm[MOVIE_NUM];
};