#pragma once

//# parameters
//# ==========
//# processed files
//# all.dta
//# all.idx
//# found 102416306 entries, out of which
//# train 1 : 94362233
//# train 2 : 1965045
//# train 3 : 1964391
//# train 6 : 0
//# probe : 1374739
//# test : 2749898

#define USER_NUM			458293
#define MOVIE_NUM			17770
#define TOTAL_NUM_RATINGS	102416306 
#define NUM_TRAIN_RATINGS	1964391		// set with type = 3		// 96326624		// set with type = 1 or 3 
#define NUM_VALID_RATINGS	1965045		// set with type = 2
#define NUM_PROBE_RATINGS	1374739

// Nearest-neighbors parameters
#define NUMBER_OF_UNIQUE_WEIGHTS 157877565 // 1/2 of off-diagnoal entries of 17770 x 17770 matrix

#define SEEDRANGE_BIAS 0.05
#define SEEDRANGE_CORR 0
#define SEEDRANGE_BASELINE 0

#define MEAN_RATING 3.60861
#define THRESHOLD 0
#define NUM_THREADS 8

#define REGULARIZATION_CONST_W 0.002f
#define REGULARIZATION_CONST_C 0.002f

#define LEARNING_RATE 0.0001

#define B_LEARNING_RATIO 0.02 // Is this right?

#define LEARNING_DEC 0.8
#define LEARNING_INC 1.005

#define LEARNING_UP 1.5
#define LEARNING_DOWN 1.5
#define TUNE_FREQ 300000

#define ITERATION_NUM 10

#define RUN_NAME "kNN"
#define RUN_COMMENT "random init [0,0.1]"


// Data-structures

struct FileEntry
{
	unsigned int	user;
	unsigned short	movie;
	unsigned short	date;
	unsigned char	rating;
	unsigned char	category;
};

struct DataEntry
{
	float rating;
	unsigned int	user_ptr;
	unsigned int	movie_ptr;
};

struct UserStats
{
	unsigned short	allRatedMoviesByUserSize_N[USER_NUM];
	float			invSqRootSizeN[USER_NUM];
	unsigned short	allRatedMoviesByUser_N[TOTAL_NUM_RATINGS];
	unsigned short	allRatedMoviesWithRatingByUserSize_R[USER_NUM];
	float			invSqRootSizeR[USER_NUM];
	unsigned short	allRatedMoviesWithRatingByUser_R[NUM_TRAIN_RATINGS];
};

struct Model
{
	float			v_bu[USER_NUM];
	float			v_bm[MOVIE_NUM];
	float			m_corr_W[NUMBER_OF_UNIQUE_WEIGHTS];
	float			m_baseline_C[NUMBER_OF_UNIQUE_WEIGHTS];
};
