#pragma once

#define SVD_dim 40
#define USER_NUM 458293
#define MOVIE_NUM 17770

#define MEAN_RATING 3.60861
#define THRESHOLD 0
#define NUM_THREADS 4

#define REGULARIZATION_CONST_U 0.000f
#define REGULARIZATION_CONST_M 0.000f

#define LEARNING_RATE 0.00005
#define LEARNING_DEC 0.8
#define LEARNING_INC 1.005

#define LEARNING_UP 1.5
#define LEARNING_DOWN 1.5
#define TUNE_FREQ 30

#define ITERATION_NUM 10

#define RUN_NAME "run4"

#define RUN_COMMENT "random init [-0.1,0.1]"

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