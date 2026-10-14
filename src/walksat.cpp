/*
 Copyright 2018 Raffaele Marino

 This file is part of BSP (BackTracking Survey Propagation).

 BSP is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 BSP is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with BSP; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 *  walksat.cpp
 *  RANDOM-K-SAT-BSP
 *
 *
 */

/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/

/*COPYRIGHTS OF WALKSAT: BART SELMAN AND HENRY KAUTZ*/
/*WALKSAT VERSION 35 2003*/
/*VERSION C++ by GIORGIO PARISI 01/01/2003*/
/*MINIMAL MODIFICATION FOR COMPATIBILITY*/

/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/



/* walksat version 35 */
/* version 1 by Bram Cohen 7/93 */
/* versions 2 - 35  by Henry Kautz */
/* version C++ by Giorgio Parisi 1/1/2003 minimal modifiications for compatibility
 / ********************************** */
/* Compilation flags                */
/************************************/

/* If the constant HUGE is set, then dynamic arrays are used instead of static arrays.
 This allows very large or very small problems to be handled, without
 having to adjust the constants MAXATOM and/or MAXCLAUSE.  However, on some
 architectures (e.g. SGI) the compiled program is about 25% slower, because not
 as many optimizations can be performed. */

// #define HUGE 1

/* If the constant NT is set, then the program is modified to compile under Windows NT.
 Currently, this eliminates the timing functionality. */



#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <bsp/random.hpp>
#include <bsp/run.hpp>
int WalkSat(std::vector <std::vector<bool> > & sol,int argc, char * argv[]);

#define NT

/************************************/
/* Standard includes                */
/************************************/

/*#include <stdio.h>*/
#include <string.h>
#include <stdlib.h>
#include <math.h>
/*#include <sys/types.h>*/
#include <limits.h>
#include <signal.h>

#ifndef NT
/*#include <sys/times.h>
 #include <sys/time.h>*/
#endif

#ifndef CLK_TCK
#define CLK_TCK 60
#endif

#ifdef NT
//#define sRandom(seed) {myrand=seed;}
#endif

/************************************/
/* Constant parameters              */
/************************************/

#define MAXATOM 1000000		/* maximum possible number of atoms */

#ifdef Huge
#define STOREBLOCK 1000	/* size of block to malloc each time */
#else
#define STOREBLOCK 1000	/* size of block to malloc each time */
#define MAXCLAUSE 5000000	/* maximum possible number of clauses */
#endif

#define TRUE 1
#define FALSE 0

#define MAXLENGTH 500           /* maximum number of literals which can be in any clause */


/************************************/
/* Internal constants               */
/************************************/

enum heuristics { RANDOM, BEST, TABU, NOVELTY, RNOVELTY };

#define NOVALUE -1
#define INIT_PARTIAL 1
#define HISTMAX 101		/* length of histogram of tail */

#define Var(CLAUSE, POSITION) (ABS(clause[CLAUSE][POSITION]))

static int scratch;
#define ABS(x) ((scratch=(x))>0?(scratch):(-scratch))

#define BIG 100000000



/************************************/
/* Main data structures             */
/************************************/

/* Atoms start at 1 */
/* Not a is recorded as -1 * a */
/* One dimensional arrays are statically allocated. */
/* Two dimensional arrays are dynamically allocated in */
/* the second dimension only.  */

int numatom;
int numclause;
int numliterals;

#ifdef Huge
int ** clause;			/* clauses to be satisfied */
/* indexed as clause[clause_num][literal_num] */
int * size;			/* length of each clause */
int * fals;			/* clauses which are false */
int * lowfalse;
int * wherefalse;		/* where each clause is listed in false */
int * numtruelit;		/* number of true literals in each clause */
#else
int * clause[MAXCLAUSE];	/* clauses to be satisfied */
/* indexed as clause[clause_num][literal_num] */
int size[MAXCLAUSE];		/* length of each clause */
int fals[MAXCLAUSE];		/* clauses which are false */
int lowfalse[MAXCLAUSE];
int wherefalse[MAXCLAUSE];	/* where each clause is listed in false */
int numtruelit[MAXCLAUSE];	/* number of true literals in each clause */
int seed_1;
#endif

int *occurence[2*MAXATOM+1];	/* where each literal occurs */
/* indexed as occurence[literal+MAXATOM][occurence_num] */

int numoccurence[2*MAXATOM+1];	/* number of times each literal occurs */


int atom[MAXATOM+1];		/* value of each atom */
int lowatom[MAXATOM+1];
int solution[MAXATOM+1];

int changed[MAXATOM+1];		/* step at which atom was last flipped */

int breakcount[MAXATOM+1];	/* number of clauses that become unsat if var if flipped */
int makecount[MAXATOM+1];	/* number of clauses that become sat if var if flipped */

int numfalse;			/* number of false clauses */

/************************************/
/* Global flags and parameters      */
/************************************/
FILE * infile;
int abort_flag;

int heuristic = BEST;		/* heuristic to be used */
int numerator = NOVALUE;	/* make Random flip with numerator/denominator frequency */
int denominator = 100;
int tabu_length;		/* length of tabu list */

long int numflip;		/* number of changes so far */
long int numnullflip;		/*  number of times a clause was picked, but no  */
/*  variable from it was flipped  */
int numrun = 10;
int cutoff = 100000;
int base_cutoff = 100000;
int target = 0;
int numtry = 0;			/* total attempts at solutions */
int numsol = NOVALUE;		/* stop after this many tries succeeds */
int superlinear = FALSE;

int makeflag = FALSE;		/* set to true by heuristics that require the make values to be calculated */

/* Histogram of tail */

long int tailhist[HISTMAX];	/* histogram of num unsat in tail of run */
long histtotal;
int tail = 3;
int tail_start_flip;

/* Printing options */

int printonlysol = FALSE;
int printsolcnf = FALSE;
int printfalse = FALSE;
int printlow = FALSE;
int printhist = FALSE;
int printtrace = FALSE;
int trace_assign = FALSE;

/* Initialization options */

char initfile[100] = { 0 };
int initoptions = FALSE;

/* Randomization */

//int seed;			/* seed for Random */

/* Statistics */

double expertime;
long flips_this_solution;
long int lowbad;		/* lowest number of bad clauses during try */
long int totalflip = 0;		/* total number of flips in all tries so far */
long int totalsuccessflip = 0;	/* total number of flips in all tries which succeeded so far */
int numsuccesstry = 0;		/* total found solutions */

long x;
long integer_sum_x = 0;
double sum_x = 0.0;
double sum_x_squared = 0.0;
double mean_x;
double second_moment_x;
double variance_x;
double std_dev_x;
double std_error_mean_x;
double seconds_per_flip;
int r;
int sum_r = 0;
double sum_r_squared = 0.0;
double mean_r;
double variance_r;
double std_dev_r;
double std_error_mean_r;

double avgfalse;
double sumfalse;
double sumfalse_squared;
double second_moment_avgfalse, variance_avgfalse, std_dev_avgfalse, ratio_avgfalse;
//double std_dev_avgfalse;
double f;
double sample_size;

double sum_avgfalse = 0.0;
double sum_std_dev_avgfalse = 0.0;
double mean_avgfalse;
double mean_std_dev_avgfalse;
int number_sampled_runs = 0;
double ratio_mean_avgfalse;

double suc_sum_avgfalse = 0.0;
double suc_sum_std_dev_avgfalse = 0.0;
double suc_mean_avgfalse;
double suc_mean_std_dev_avgfalse;
int suc_number_sampled_runs = 0;
double suc_ratio_mean_avgfalse;

double nonsuc_sum_avgfalse = 0.0;
double nonsuc_sum_std_dev_avgfalse = 0.0;
double nonsuc_mean_avgfalse;
double nonsuc_mean_std_dev_avgfalse;
int nonsuc_number_sampled_runs = 0;
double nonsuc_ratio_mean_avgfalse;

/* Hamming calcualations */

char hamming_target_file[512] = { 0 };
char hamming_data_file[512] = { 0 };
int hamming_sample_freq;
int hamming_flag = FALSE;
int hamming_distance;
int hamming_target[MAXATOM+1];
void read_hamming_file(char initfile[]);
void open_hamming_data(char initfile[]);
int calc_hamming_dist(int atom[], int hamming_target[], int numatom);
FILE * hamming_fp;

/* Noise level */
int samplefreq = 1;

/************************************/
/* Forward declarations             */
/************************************/

void parse_parameters(int argc,char *argv[]);
void print_parameters(int argc, char * argv[]);

int pickRandom(void);
int pickbest(void);
int picktabu(void);
int picknovelty(void);
int pickrnovelty(void);
const char * heuristic_names[] = { "Random", "best", "tabu", "novelty", "rnovelty" };
static int (*pickcode[])(void) =
{pickRandom, pickbest, picktabu, picknovelty, pickrnovelty};

double elapsed_seconds(void);
int countunsat(void);
void scanone(int argc, char *argv[], int i, int *varptr);
void init(char initfile[], int initoptions);
void initprob(void);                 /* create a new problem */
void flipatom(int toflip);           /* changes the assignment of the given literal */

void print_false_clauses(long int lowbad);
void save_false_clauses(long int lowbad);
void print_low_assign(long int lowbad);
void save_low_assign(void);
void save_solution(void);

void print_current_assign(void);
void handle_interrupt(int sig);

long super(int i);


void print_statistics_header(void);
void initialize_statistics(void);
void update_statistics_start_try(void);
void print_statistics_start_flip(void);
void update_and_print_statistics_end_try(std::vector <std::vector<bool> > & sol);
void update_statistics_end_flip(void);
void print_statistics_final();
void print_sol_cnf(std::vector <bool> & sol);

/************************************/
/* Main                             */
/************************************/


/* Track malloc blocks from initprob so WalkSat can free them on exit. */
static std::vector<void*> walksat_blocks;

static void *walksat_alloc(size_t bytes)
{
    void *p = malloc(bytes);
    if (p == NULL) {
        bsp::console_fprintf(stderr, "ERROR - out of memory in WalkSat\n");
        throw std::runtime_error("WalkSAT operation failed");
    }
    walksat_blocks.push_back(p);
    return p;
}

static void walksat_free_blocks(void)
{
    for (size_t i = 0; i < walksat_blocks.size(); ++i)
        free(walksat_blocks[i]);
    walksat_blocks.clear();
}

static void walksat_cleanup(void)
{
    walksat_free_blocks();
    if (infile != NULL && infile != stdin) {
        fclose(infile);
        infile = stdin;
    }
}

static const std::vector<std::vector<int>> *memory_clauses = nullptr;
static unsigned memory_n = 0;
static void reset_walksat() {
    heuristic = BEST;
    numerator = NOVALUE;
    denominator = 100;
    tabu_length = 0;
    numflip = 0;
    numnullflip = 0;
    numrun = 10;
    cutoff = 100000;
    base_cutoff = 100000;
    target = 0;
    numtry = 0;
    numsol = NOVALUE;
    superlinear = FALSE;
    makeflag = FALSE;
    histtotal = 0;
    tail = 3;
    tail_start_flip = 0;
    printonlysol = FALSE;
    printsolcnf = FALSE;
    printfalse = FALSE;
    printlow = FALSE;
    printhist = FALSE;
    printtrace = FALSE;
    trace_assign = FALSE;
    initoptions = FALSE;
    expertime = 0;
    flips_this_solution = 0;
    lowbad = 0;
    totalflip = 0;
    totalsuccessflip = 0;
    numsuccesstry = 0;
    x = 0;
    integer_sum_x = 0;
    sum_x = 0.0;
    sum_x_squared = 0.0;
    mean_x = 0;
    second_moment_x = 0;
    variance_x = 0;
    std_dev_x = 0;
    std_error_mean_x = 0;
    seconds_per_flip = 0;
    r = 0;
    sum_r = 0;
    sum_r_squared = 0.0;
    mean_r = 0;
    variance_r = 0;
    std_dev_r = 0;
    std_error_mean_r = 0;
    avgfalse = 0;
    sumfalse = 0;
    sumfalse_squared = 0;
    second_moment_avgfalse = 0;
    variance_avgfalse = 0;
    std_dev_avgfalse = 0;
    ratio_avgfalse = 0;
    f = 0;
    sample_size = 0;
    sum_avgfalse = 0.0;
    sum_std_dev_avgfalse = 0.0;
    mean_avgfalse = 0;
    mean_std_dev_avgfalse = 0;
    number_sampled_runs = 0;
    ratio_mean_avgfalse = 0;
    suc_sum_avgfalse = 0.0;
    suc_sum_std_dev_avgfalse = 0.0;
    suc_mean_avgfalse = 0;
    suc_mean_std_dev_avgfalse = 0;
    suc_number_sampled_runs = 0;
    suc_ratio_mean_avgfalse = 0;
    nonsuc_sum_avgfalse = 0.0;
    nonsuc_sum_std_dev_avgfalse = 0.0;
    nonsuc_mean_avgfalse = 0;
    nonsuc_mean_std_dev_avgfalse = 0;
    nonsuc_number_sampled_runs = 0;
    nonsuc_ratio_mean_avgfalse = 0;
    hamming_sample_freq = 0;
    hamming_flag = FALSE;
    hamming_distance = 0;
    samplefreq = 1;
    initfile[0]=0; hamming_target_file[0]=0; hamming_data_file[0]=0;
}
int WalkSat(std::vector <std::vector<bool> > & sol,int argc, char * argv[])
{
    walksat_free_blocks(); /* free blocks from a previous WalkSat call, if any */
    reset_walksat();
    struct Cleanup { ~Cleanup() { walksat_cleanup(); } } cleanup;

    parse_parameters(argc, argv);
    //    sRandom(seed);
    print_parameters(argc, argv);
    initprob();
    initialize_statistics();
    print_statistics_header();
    abort_flag = FALSE;
    (void) elapsed_seconds();

    while (! abort_flag && numsuccesstry < numsol && numtry < numrun) {
        bsp::check_cancelled();
        numtry++;
        init(initfile, initoptions);
        update_statistics_start_try();
        numflip = 0;

        if (superlinear) cutoff = base_cutoff * super(numtry);

        while((numfalse > target) && (numflip < cutoff)) {
            if ((numflip & 1023) == 0) bsp::check_cancelled();
            print_statistics_start_flip();
            numflip++;
            flipatom((pickcode[heuristic])());
            update_statistics_end_flip();
        }
        update_and_print_statistics_end_try(sol);
    }
    expertime = elapsed_seconds();
    print_statistics_final();
    walksat_cleanup();
    return 0;
}



void parse_parameters(int argc,char *argv[])
{
    int i;
    int temp;
    /* walksat -seed 1 -solcnf -cutoff %i < %s > %s",cutoff,formula,output"
     -seed 1 -solcnf  -cutoff  1000000  input output*/
    infile= stdin;
    for (i=1; i < argc; i++)
    {
        if (argv[i][0] != '-' && infile == stdin) {
            infile = fopen(argv[i],"r");
            if (infile == NULL) {
                bsp::console_fprintf(stderr, "Cannot open file named %s\n", argv[i]);
                throw std::runtime_error("WalkSAT operation failed");
            }
            ++i;
        } else if (strcmp(argv[i],"-seed") == 0) {
            scanone(argc,argv,++i,&seed_1);
            seed_1=myrand;
        }
        else if (strcmp(argv[i],"-hist") == 0)
            printhist = TRUE;
        else if (strcmp(argv[i],"-cutoff") == 0)
            scanone(argc,argv,++i,&cutoff);
        else if (strcmp(argv[i],"-Random") == 0)
            heuristic = RANDOM;
        else if (strcmp(argv[i],"-novelty") == 0) {
            heuristic = NOVELTY;
            makeflag = TRUE;
        }
        else if (strcmp(argv[i],"-rnovelty") == 0) {
            heuristic = RNOVELTY;
            makeflag = TRUE;
        }
        else if (strcmp(argv[i],"-best") == 0)
            heuristic = BEST;
        else if (strcmp(argv[i],"-noise") == 0) {
            scanone(argc,argv,++i,&numerator);
            if (i < argc-1 && sscanf(argv[i+1],"%i",&temp)==1) {
                denominator = temp;
                i++;
            }
        }
        else if (strcmp(argv[i],"-init") == 0  && i < argc-1)
            sscanf(argv[++i], " %s", initfile);
        else if (strcmp(argv[i],"-hamming") == 0  && i < argc-3) {
            sscanf(argv[++i], " %s", hamming_target_file);
            sscanf(argv[++i], " %s", hamming_data_file);
            sscanf(argv[++i], " %i", &hamming_sample_freq);
            hamming_flag = TRUE;
            numrun = 1;
        }
        else if (strcmp(argv[i],"-partial") == 0)
            initoptions = INIT_PARTIAL;
        else if (strcmp(argv[i],"-super") == 0)
            superlinear = TRUE;
        else if (strcmp(argv[i],"-tries") == 0)
            scanone(argc,argv,++i,&numrun);
        else if (strcmp(argv[i],"-target") == 0)
            scanone(argc,argv,++i,&target);
        else if (strcmp(argv[i],"-tail") == 0)
            scanone(argc,argv,++i,&tail);
        else if (strcmp(argv[i],"-sample") == 0)
            scanone(argc,argv,++i,&samplefreq);
        else if (strcmp(argv[i],"-tabu") == 0)
        {
            scanone(argc,argv,++i,&tabu_length);
            heuristic = TABU;
        }
        else if (strcmp(argv[i],"-low") == 0)
            printlow = TRUE;
        else if (strcmp(argv[i],"-sol") == 0)
        {
            printonlysol = TRUE;
            printlow = TRUE;
        }
        else if (strcmp(argv[i],"-solcnf") == 0)
        {
            printsolcnf = TRUE;
            if (numsol == NOVALUE) numsol = 1;
        }
        else if (strcmp(argv[i],"-bad") == 0)
            printfalse = TRUE;
        else if (strcmp(argv[i],"-numsol") == 0)
            scanone(argc,argv,++i,&numsol);
        else if (strcmp(argv[i],"-trace") == 0)
            scanone(argc,argv,++i,&printtrace);
        else if (strcmp(argv[i],"-assign") == 0) {
            scanone(argc,argv,++i,&printtrace);
            trace_assign = TRUE;
        }
        else
        {
            if (strcmp(argv[i],"-help")!=0 && strcmp(argv[i],"-h")!=0 )
                bsp::console_fprintf(stderr, "Bad argument %s\n", argv[i]);
            bsp::console_fprintf(stderr, "General parameters:\n");
            bsp::console_fprintf(stderr, "  -seed N -cutoff N -tries N -target N\n");
            bsp::console_fprintf(stderr, "  -numsol N = stop after finding N solutions\n");
            bsp::console_fprintf(stderr, "  -init FILE = set vars not included in FILE to false\n");
            bsp::console_fprintf(stderr, "  -partial FILE = set vars not included in FILE Randomly\n");
            bsp::console_fprintf(stderr, "Heuristics:\n");
            bsp::console_fprintf(stderr, "  -Random -best -tabu N -novelty -rnovelty\n");
            bsp::console_fprintf(stderr, "  -noise N or -noise N M (default M = 100)\n");
            bsp::console_fprintf(stderr, "Printing:\n");
            bsp::console_fprintf(stderr, "  -hamming TARGET_FILE DATA_FILE SAMPLE_FREQUENCY\n");
            bsp::console_fprintf(stderr, "  -trace N = print statistics every N flips\n");
            bsp::console_fprintf(stderr, "  -assign N = print assignments at flip N, 2N, ...\n");
            bsp::console_fprintf(stderr, "  -sol = print satisfying assignments\n");
            bsp::console_fprintf(stderr, "  -low = print lowest assignment each try\n");
            bsp::console_fprintf(stderr, "  -bad = print unsat clauses each try\n");
            bsp::console_fprintf(stderr, "  -tail N = assume tail begins at nvars*N\n");
            bsp::console_fprintf(stderr, "  -solcnf = print sat assign in cnf format, and exit\n");
            bsp::console_fprintf(stderr, "  -sample N = sample noise level every N flips\n");
            throw std::runtime_error("WalkSAT operation failed");
        }
    }
    base_cutoff = cutoff;
    if (numsol==NOVALUE || numsol>numrun) numsol = numrun;
    if (numerator==NOVALUE) {
        switch(heuristic) {
        case BEST:
        case NOVELTY:
        case RNOVELTY:
            numerator = 50;
            break;
        default:
            numerator = 0;
            break;
        }
    }
}


void print_parameters(int argc, char * argv[])
{
    int i;

#ifdef Huge
    bsp::console_printf("walksat version 35 (Huge)\n");
#else
    bsp::console_printf("walksat version 35\n");
#endif
    bsp::console_printf("command line =");
    for (i=0; i < argc; i++) {
        bsp::console_printf(" %s", argv[i]);
    }
    bsp::console_printf("\n");
    bsp::console_printf("seed = %i\n",seed_1);
    bsp::console_printf("cutoff = %i\n",cutoff);
    bsp::console_printf("tries = %i\n",numrun);

    bsp::console_printf("heuristic = ");
    switch(heuristic)
    {
    case TABU:
        bsp::console_printf("tabu %d", tabu_length);
        break;
    default:
        bsp::console_printf("%s", heuristic_names[heuristic]);
        break;
    }
    if (numerator>0) {
        bsp::console_printf(", noise %d / %d", numerator, denominator);
    }
    bsp::console_printf("\n");
}

void print_statistics_header(void)
{
    bsp::console_printf("numatom = %i, numclause = %i, numliterals = %i\n",numatom,numclause,numliterals);
    bsp::console_printf("wff read in\n\n");
    bsp::console_printf("    lowest     final       avg     noise     noise     total                 avg        mean        mean\n");
    bsp::console_printf("    #unsat    #unsat     noise   std dev     ratio     flips              length       flips       flips\n");
    bsp::console_printf("      this      this      this      this      this      this   success   success       until         std\n");
    bsp::console_printf("       try       try       try       try       try       try      rate     tries      assign         dev\n\n");

    fflush(stdout);

}

void initialize_statistics(void)
{
    x = 0;
    r = 0;
    if (hamming_flag) {
        read_hamming_file(hamming_target_file);
        open_hamming_data(hamming_data_file);
    }
    tail_start_flip = tail * numatom;
    bsp::console_printf("tail starts after flip = %i\n", tail_start_flip);
    numnullflip = 0;


}

void update_statistics_start_try(void)
{
    int i;

    lowbad = numfalse;

    sample_size = 0;
    sumfalse = 0.0;
    sumfalse_squared = 0.0;

    for (i=0; i<HISTMAX; i++)
        tailhist[i] = 0;
    if (tail_start_flip == 0) {
        tailhist[numfalse < HISTMAX ? numfalse : HISTMAX - 1] ++;
    }

    if (printfalse) save_false_clauses(lowbad);
    if (printlow) save_low_assign();
}

void print_statistics_start_flip(void)
{
    if (printtrace && (numflip % printtrace == 0)) {
        bsp::console_printf(" %9li %9i                     %9li\n", lowbad,numfalse,numflip);
        if (trace_assign)
            print_current_assign();
        fflush(stdout);
    }
}


void update_and_print_statistics_end_try(std::vector <std::vector<bool> > & sol)
{
    int i;
    int j;

    totalflip += numflip;
    x += numflip;
    r ++;

    if (sample_size > 0) {
        avgfalse = sumfalse/sample_size;
        second_moment_avgfalse = sumfalse_squared / sample_size;
        variance_avgfalse = second_moment_avgfalse - (avgfalse * avgfalse);
        if (sample_size > 1) {
            variance_avgfalse = (variance_avgfalse * sample_size)/(sample_size - 1);
        }
        std_dev_avgfalse = sqrt(variance_avgfalse);

        ratio_avgfalse = avgfalse / std_dev_avgfalse;

        sum_avgfalse += avgfalse;
        sum_std_dev_avgfalse += std_dev_avgfalse;
        number_sampled_runs += 1;

        if (numfalse == 0) {
            suc_number_sampled_runs += 1;
            suc_sum_avgfalse += avgfalse;
            suc_sum_std_dev_avgfalse += std_dev_avgfalse;
        }
        else {
            nonsuc_number_sampled_runs += 1;
            nonsuc_sum_avgfalse += avgfalse;
            nonsuc_sum_std_dev_avgfalse += std_dev_avgfalse;
        }
    }
    else {
        avgfalse = 0;
        variance_avgfalse = 0;
        std_dev_avgfalse = 0;
        ratio_avgfalse = 0;
    }

    if(numfalse == 0) {

        save_solution();
        std::vector <bool> next;
        print_sol_cnf( next);
        sol.push_back(next);
        numsuccesstry++;

        totalsuccessflip += numflip;
        integer_sum_x += x;
        sum_x = (double) integer_sum_x;
        sum_x_squared += ((double)x)*((double)x);
        mean_x = sum_x / numsuccesstry;
        if (numsuccesstry > 1) {
            second_moment_x = sum_x_squared / numsuccesstry;
            variance_x = second_moment_x - (mean_x * mean_x);
            /* Adjustment for small small sample size */
            variance_x = (variance_x * numsuccesstry)/(numsuccesstry - 1);
            std_dev_x = sqrt(variance_x);
            std_error_mean_x = std_dev_x / sqrt((double)numsuccesstry);
        }
        sum_r += r;
        mean_r = ((double)sum_r)/numsuccesstry;
        sum_r_squared += ((double)r)*((double)r);

        x = 0;
        r = 0;
    }

    bsp::console_printf(" %9li %9i %9.2f %9.2f %9.2f %9li %9i",
           lowbad,numfalse,avgfalse, std_dev_avgfalse,ratio_avgfalse,numflip, (numsuccesstry*100)/numtry);
    if (numsuccesstry > 0) {
        bsp::console_printf(" %9li", totalsuccessflip/numsuccesstry);
        bsp::console_printf(" %11.2f", mean_x);
        if (numsuccesstry > 1) {
            bsp::console_printf(" %11.2f", std_dev_x);
        }
    }
    bsp::console_printf("\n");

    if (printhist) {
        bsp::console_printf("histogram: ");
        for (j=HISTMAX-1; tailhist[j] == 0; j--) ;
        for (i=0; i<=j; i++) {
            bsp::console_printf(" %li(%i)", tailhist[i], i);
            if ((i+1) % 10 == 0) bsp::console_printf("\n           ");
        }
        if (j==HISTMAX-1) bsp::console_printf(" +++");
        bsp::console_printf("\n");
    }

    if (numfalse>0 && printfalse)
        print_false_clauses(lowbad);
    if (printlow && (!printonlysol || numfalse >= target))
        print_low_assign(lowbad);

    if(numfalse == 0 && countunsat() != 0) {
        bsp::console_fprintf(stderr, "Program error, verification of solution fails!\n");
        throw std::runtime_error("WalkSAT operation failed");
    }

    fflush(stdout);
}

void update_statistics_end_flip(void)
{
    if (numfalse < lowbad) {
        lowbad = numfalse;
        if (printfalse) save_false_clauses(lowbad);
        if (printlow) save_low_assign();
    }
    if (numflip >= tail_start_flip) {
        tailhist[(numfalse < HISTMAX) ? numfalse : (HISTMAX - 1)] ++;
        if ((numflip % samplefreq) == 0) {
            sumfalse += numfalse;
            sumfalse_squared += numfalse * numfalse;
            sample_size ++;
        }
    }
}

void print_statistics_final()
{
    seconds_per_flip = expertime / totalflip;
    bsp::console_printf("\ntotal elapsed seconds = %f\n", expertime);
    bsp::console_printf("average flips per second = %li\n", (long)(totalflip/expertime));
    if (heuristic == TABU)
        bsp::console_printf("proportion null flips = %f\n", ((double)numnullflip)/totalflip);
    bsp::console_printf("number solutions found = %d\n", numsuccesstry);
    bsp::console_printf("final success rate = %f\n", ((double)numsuccesstry * 100.0)/numtry);
    bsp::console_printf("average length successful tries = %li\n", numsuccesstry ? (totalsuccessflip/numsuccesstry) : 0);
    if (numsuccesstry > 0)
    {
        bsp::console_printf("mean flips until assign = %f\n", mean_x);
        if (numsuccesstry>1) {
            bsp::console_printf("  variance = %f\n", variance_x);
            bsp::console_printf("  standard deviation = %f\n", std_dev_x);
            bsp::console_printf("  standard error of mean = %f\n", std_error_mean_x);
        }
        bsp::console_printf("mean seconds until assign = %f\n", mean_x * seconds_per_flip);
        if (numsuccesstry>1) {
            bsp::console_printf("  variance = %f\n", variance_x * seconds_per_flip * seconds_per_flip);
            bsp::console_printf("  standard deviation = %f\n", std_dev_x * seconds_per_flip);
            bsp::console_printf("  standard error of mean = %f\n", std_error_mean_x * seconds_per_flip);
        }
        bsp::console_printf("mean restarts until assign = %f\n", mean_r);
        if (numsuccesstry>1) {
            variance_r = (sum_r_squared / numsuccesstry) - (mean_r * mean_r);
            if (numsuccesstry > 1) variance_r = (variance_r * numsuccesstry)/(numsuccesstry - 1);
            std_dev_r = sqrt(variance_r);
            std_error_mean_r = std_dev_r / sqrt((double)numsuccesstry);
            bsp::console_printf("  variance = %f\n", variance_r);
            bsp::console_printf("  standard deviation = %f\n", std_dev_r);
            bsp::console_printf("  standard error of mean = %f\n", std_error_mean_r);
        }
    }

    if (number_sampled_runs) {
        mean_avgfalse = sum_avgfalse / number_sampled_runs;
        mean_std_dev_avgfalse = sum_std_dev_avgfalse / number_sampled_runs;
        ratio_mean_avgfalse = mean_avgfalse / mean_std_dev_avgfalse;

        if (suc_number_sampled_runs) {
            suc_mean_avgfalse = suc_sum_avgfalse / suc_number_sampled_runs;
            suc_mean_std_dev_avgfalse = suc_sum_std_dev_avgfalse / suc_number_sampled_runs;
            suc_ratio_mean_avgfalse = suc_mean_avgfalse / suc_mean_std_dev_avgfalse;
        }
        else {
            suc_mean_avgfalse = 0;
            suc_mean_std_dev_avgfalse = 0;
            suc_ratio_mean_avgfalse = 0;
        }

        if (nonsuc_number_sampled_runs) {
            nonsuc_mean_avgfalse = nonsuc_sum_avgfalse / nonsuc_number_sampled_runs;
            nonsuc_mean_std_dev_avgfalse = nonsuc_sum_std_dev_avgfalse / nonsuc_number_sampled_runs;
            nonsuc_ratio_mean_avgfalse = nonsuc_mean_avgfalse / nonsuc_mean_std_dev_avgfalse;
        }
        else {
            nonsuc_mean_avgfalse = 0;
            nonsuc_mean_std_dev_avgfalse = 0;
            nonsuc_ratio_mean_avgfalse = 0;
        }

        bsp::console_printf("final noise level statistics\n");
        bsp::console_printf("    statistics over all runs:\n");
        bsp::console_printf("      overall mean average noise level = %f\n", mean_avgfalse);
        bsp::console_printf("      overall mean noise std deviation = %f\n", mean_std_dev_avgfalse);
        bsp::console_printf("      overall ratio mean noise to mean std dev = %f\n", ratio_mean_avgfalse);
        bsp::console_printf("    statistics on successful runs:\n");
        bsp::console_printf("      successful mean average noise level = %f\n", suc_mean_avgfalse);
        bsp::console_printf("      successful mean noise std deviation = %f\n", suc_mean_std_dev_avgfalse);
        bsp::console_printf("      successful ratio mean noise to mean std dev = %f\n", suc_ratio_mean_avgfalse);
        bsp::console_printf("    statistics on nonsuccessful runs:\n");
        bsp::console_printf("      nonsuccessful mean average noise level = %f\n", nonsuc_mean_avgfalse);
        bsp::console_printf("      nonsuccessful mean noise std deviation = %f\n", nonsuc_mean_std_dev_avgfalse);
        bsp::console_printf("      nonsuccessful ratio mean noise to mean std dev = %f\n", nonsuc_ratio_mean_avgfalse);
    }

    if (hamming_flag) {
        fclose(hamming_fp);
        bsp::console_printf("Final distance to hamming target = %i\n", calc_hamming_dist(atom, hamming_target, numatom));
        bsp::console_printf("Hamming distance data stored in %s\n", hamming_data_file);
    }

    if (numsuccesstry > 0) {
        bsp::console_printf("ASSIGNMENT FOUND\n");
        //	if(printsolcnf == TRUE) print_sol_cnf(sol);
    }
    else
        bsp::console_printf("ASSIGNMENT NOT FOUND\n");

}


long super(int i)
{
    long power;
    int k;

    if (i<=0) {
        bsp::console_fprintf(stderr, "bad argument super(%d)\n", i);
        throw std::runtime_error("WalkSAT operation failed");
    }
    /* let 2^k be the least power of 2 >= (i+1) */
    k = 1;
    power = 2;
    while (power < (i+1)) {
        k += 1;
        power *= 2;
    }
    if (power == (i+1)) return (power/2);
    return (super(i - (power/2) + 1));
}

void handle_interrupt()
{
    if (abort_flag) throw std::runtime_error("WalkSAT operation failed");
    abort_flag = TRUE;
}

void scanone(int argc, char *argv[], int i, int *varptr)
{
    if (i>=argc || sscanf(argv[i],"%i",varptr)!=1) {
        bsp::console_fprintf(stderr, "Bad argument %s\n",argv[i]);
        bsp::console_fprintf(stderr, "Bad argument %s\n", i<argc ? argv[i] : argv[argc-1]);
        throw std::runtime_error("WalkSAT operation failed");
    }
}

int calc_hamming_dist(int atom[], int hamming_target[], int numatom)
{
    int i;
    int dist = 0;

    for (i=1; i<=numatom; i++) {
        if (atom[i] != hamming_target[i]) dist++;
    }
    return dist;
}

void open_hamming_data(char initfile[])
{
    if ((hamming_fp = fopen(initfile, "w")) == NULL) {
        bsp::console_fprintf(stderr, "Cannot open %s for output\n", initfile);
        throw std::runtime_error("WalkSAT operation failed");
    }
}


void read_hamming_file(char initfile[])
{
    int i;			/* loop counter */
    FILE * infile;
    int lit;

    bsp::console_printf("loading hamming target file %s ...", initfile);

    if ((infile = fopen(initfile, "r")) == NULL) {
        bsp::console_fprintf(stderr, "Cannot open %s\n", initfile);
        throw std::runtime_error("WalkSAT operation failed");
    }
    i=0;
    for(i = 1; i < numatom+1; i++)
        hamming_target[i] = 0;

    while (fscanf(infile, " %d", &lit)==1) {
        if (ABS(lit)>numatom) {
            bsp::console_fprintf(stderr, "Bad hamming file %s\n", initfile);
            throw std::runtime_error("WalkSAT operation failed");
        }
        if (lit>0) hamming_target[lit]=1;
    }
    bsp::console_printf("done\n");
}


void init(char initfile[], int initoptions)
{
    int i;
    int j;
    int thetruelit;
    FILE * infile;
    int lit;

    for(i = 0; i < numclause; i++)
        numtruelit[i] = 0;
    numfalse = 0;

    for(i = 1; i < numatom+1; i++)
    {
        changed[i] = -BIG;
        breakcount[i] = 0;
        makecount[i] = 0;
    }

    if (initfile[0] && initoptions!=INIT_PARTIAL) {
        for(i = 1; i < numatom+1; i++)
            atom[i] = 0;
    }
    else {
        for(i = 1; i < numatom+1; i++) atom[i] = Random()%2;
    }

    if (initfile[0]) {
        if ((infile = fopen(initfile, "r")) == NULL) {
            bsp::console_fprintf(stderr, "Cannot open %s\n", initfile);
            throw std::runtime_error("WalkSAT operation failed");
        }
        i=0;
        while (fscanf(infile, " %d", &lit)==1) {
            i++;
            if (ABS(lit)>numatom) {
                bsp::console_fprintf(stderr, "Bad init file %s\n", initfile);
                throw std::runtime_error("WalkSAT operation failed");
            }
            if (lit<0) atom[-lit]=0;
            else atom[lit]=1;
        }
        if (i==0) {
            bsp::console_fprintf(stderr, "Bad init file %s\n", initfile);
            throw std::runtime_error("WalkSAT operation failed");
        }
        fclose(infile);
        /* bsp::console_printf("read %d values\n", i); */
    }

    /* Initialize breakcount and makecount in the following: */
    for(i = 0; i < numclause; i++)
    {
        for(j = 0; j < size[i]; j++)
        {
            if((clause[i][j] > 0) == atom[ABS(clause[i][j])])
            {
                numtruelit[i]++;
                thetruelit = clause[i][j];
            }
        }
        if(numtruelit[i] == 0)
        {
            wherefalse[i] = numfalse;
            fals[numfalse] = i;
            numfalse++;
            for(j = 0; j < size[i]; j++) {
                makecount[ABS(clause[i][j])]++;
            }
        }
        else if (numtruelit[i] == 1)
        {
            breakcount[ABS(thetruelit)]++;
        }
    }
    if (hamming_flag) {
        hamming_distance = calc_hamming_dist(atom, hamming_target, numatom);
        bsp::console_fprintf(hamming_fp, "0 %i\n", hamming_distance);
    }
}

void
print_false_clauses(long int lowbad)
{
    int i, j;
    int cl;

    bsp::console_printf("Unsatisfied clauses:\n");
    for (i=0; i<lowbad; i++) {
        cl = lowfalse[i];
        for (j=0; j<size[cl]; j++) {
            bsp::console_printf("%d ", clause[cl][j]);
        }
        bsp::console_printf("0\n");
    }
    bsp::console_printf("End unsatisfied clauses\n");
}

void
save_false_clauses(long int lowbad)
{
    int i;

    for (i=0; i<lowbad; i++)
        lowfalse[i] = fals[i];
}

void initprob(void)
{
    int i;			/* loop counter */
    int j;			/* another loop counter */
    int lastc;
    int nextc;
    int *storeptr;
    int freestore;
    int lit;

    if (memory_clauses) { numatom=memory_n; numclause=memory_clauses->size(); }
    else {
    while ((lastc = getc(infile)) == 'c')
    {
        while ((nextc = getc(infile)) != EOF && nextc != '\n') ;
    }
    ungetc(lastc,infile);
    if (fscanf(infile,"p cnf %i %i",&numatom,&numclause) != 2)
    {
        bsp::console_fprintf(stderr,"Bad input file: wrong p line\n");
        throw std::runtime_error("WalkSAT operation failed");
    }
    }
    if(numatom > MAXATOM)
    {
        bsp::console_fprintf(stderr,"ERROR - too many atoms\n");
        throw std::runtime_error("WalkSAT operation failed");
    }

#ifdef Huge
    clause = (int **) walksat_alloc(sizeof(int *)*(numclause+1));
    size = (int *) walksat_alloc(sizeof(int)*(numclause+1));
    fals = (int *) walksat_alloc(sizeof(int)*(numclause+1));
    lowfalse = (int *) walksat_alloc(sizeof(int)*(numclause+1));
    wherefalse = (int *) walksat_alloc(sizeof(int)*(numclause+1));
    numtruelit = (int *) walksat_alloc(sizeof(int)*(numclause+1));
#else
    if(numclause > MAXCLAUSE)
    {
        bsp::console_fprintf(stderr,"ERROR - too many clauses\n");
        throw std::runtime_error("WalkSAT operation failed");
    }
#endif
    freestore = 0;
    numliterals = 0;
    for(i = 0; i < 2*MAXATOM+1; i++)
        numoccurence[i] = 0;
    for(i = 0; i < numclause; i++)
    {
        size[i] = -1;
        if (freestore < MAXLENGTH)
        {
            storeptr = (int *) walksat_alloc( sizeof(int) * STOREBLOCK );
            freestore = STOREBLOCK;
            //bsp::console_fprintf(stderr,"allocating memory...\n");
        }
        clause[i] = storeptr;
        do
        {
            size[i]++;
            if(size[i] > MAXLENGTH)
            {
                bsp::console_printf("ERROR - clause too long\n");
                throw std::runtime_error("WalkSAT operation failed");
            }
            if (memory_clauses) lit = size[i] < int((*memory_clauses)[i].size()) ? (*memory_clauses)[i][size[i]] : 0;
            if (!memory_clauses && fscanf(infile,"%i ",&lit) != 1)
            {
                bsp::console_fprintf(stderr, "Bad input file\n");
                throw std::runtime_error("WalkSAT operation failed");
            }
            if(lit != 0)
            {
                *(storeptr++) = lit; /* clause[i][size[i]] = j; */
                freestore--;
                numliterals++;
                numoccurence[lit+MAXATOM]++;
            }
        }
        while(lit != 0);
    }
    if(numclause > 0 && size[0] == 0)
    {
        bsp::console_fprintf(stderr,"ERROR - incorrect problem format or extraneous characters\n");
        throw std::runtime_error("WalkSAT operation failed");
    }

    for(i = 0; i < 2*MAXATOM+1; i++)
    {
        if (freestore < numoccurence[i])
        {
            storeptr = (int *) walksat_alloc( sizeof(int) * STOREBLOCK );
            freestore = STOREBLOCK;
            bsp::console_fprintf(stderr,"allocating memory...\n");
        }
        occurence[i] = storeptr;
        freestore -= numoccurence[i];
        storeptr += numoccurence[i];
        numoccurence[i] = 0;
    }

    for(i = 0; i < numclause; i++)
    {
        for(j = 0; j < size[i]; j++)
        {
            occurence[clause[i][j]+MAXATOM]
            [numoccurence[clause[i][j]+MAXATOM]] = i;
            numoccurence[clause[i][j]+MAXATOM]++;
        }
    }
}


void flipatom(int toflip)
{
    int i, j;			/* loop counter */
    int toenforce;		/* literal to enforce */
    int cli;
    int lit;
    int numocc;
    int sz;
    int * litptr;
    int * occptr;

    /* bsp::console_printf("flipping %i\n", toflip); */

    if (toflip == NOVALUE) {
        numnullflip++;
        return;
    }

    changed[toflip] = numflip;
    if(atom[toflip] > 0)
        toenforce = -toflip;
    else
        toenforce = toflip;
    atom[toflip] = 1-atom[toflip];

    if (hamming_flag) {
        if (atom[toflip] == hamming_target[toflip])
            hamming_distance--;
        else
            hamming_distance++;
        if ((numflip % hamming_sample_freq) == 0)
            bsp::console_fprintf(hamming_fp, "%li %i\n", numflip, hamming_distance);
    }

    numocc = numoccurence[MAXATOM-toenforce];
    occptr = occurence[MAXATOM-toenforce];
    for(i = 0; i < numocc ; i++)
    {
        /* cli = occurence[MAXATOM-toenforce][i]; */
        cli = *(occptr++);

        if (--numtruelit[cli] == 0) {
            fals[numfalse] = cli;
            wherefalse[cli] = numfalse;
            numfalse++;
            /* Decrement toflip's breakcount */
            breakcount[toflip]--;

            if (makeflag) {
                /* Increment the makecount of all vars in the clause */
                sz = size[cli];
                litptr = clause[cli];
                for (j=0; j<sz; j++) {
                    /* lit = clause[cli][j]; */
                    lit = *(litptr++);
                    makecount[ABS(lit)]++;
                }
            }
        }
        else if (numtruelit[cli] == 1) {
            /* Find the lit in this clause that makes it true, and inc its breakcount */
            sz = size[cli];
            litptr = clause[cli];
            for (j=0; j<sz; j++) {
                /* lit = clause[cli][j]; */
                lit = *(litptr++);
                if((lit > 0) == atom[ABS(lit)]) {
                    breakcount[ABS(lit)]++;
                    break;
                }
            }
        }
    }

    numocc = numoccurence[MAXATOM+toenforce];
    occptr = occurence[MAXATOM+toenforce];
    for(i = 0; i < numocc; i++)
    {
        /* cli = occurence[MAXATOM+toenforce][i]; */
        cli = *(occptr++);

        if (++numtruelit[cli] == 1) {
            numfalse--;
            fals[wherefalse[cli]] =
                fals[numfalse];
            wherefalse[fals[numfalse]] =
                wherefalse[cli];
            /* Increment toflip's breakcount */
            breakcount[toflip]++;

            if (makeflag) {
                /* Decrement the makecount of all vars in the clause */
                sz = size[cli];
                litptr = clause[cli];
                for (j=0; j<sz; j++) {
                    /* lit = clause[cli][j]; */
                    lit = *(litptr++);
                    makecount[ABS(lit)]--;
                }
            }
        }
        else if (numtruelit[cli] == 2) {
            /* Find the lit in this clause other than toflip that makes it true,
             and decrement its breakcount */
            sz = size[cli];
            litptr = clause[cli];
            for (j=0; j<sz; j++) {
                /* lit = clause[cli][j]; */
                lit = *(litptr++);
                if( ((lit > 0) == atom[ABS(lit)]) &&
                        (toflip != ABS(lit)) ) {
                    breakcount[ABS(lit)]--;
                    break;
                }
            }
        }
    }
}

int pickRandom(void)
{
    int tofix;
    FRAND
    tofix = fals[Random()%numfalse];
    FRAND
    return Var(tofix, Random()%size[tofix]);
}


int pickbest(void)
{
    int numbreak;
    int tofix;
    int clausesize;
    int i;
    int best[MAXLENGTH];	/* best possibility so far */
    int numbest;	/* how many are tied for best */
    int bestvalue;	/* best value so far */
    int var;
    FRAND
    tofix = fals[Random()%numfalse];
    clausesize = size[tofix];
    numbest = 0;
    bestvalue = BIG;

    for (i=0; i< clausesize; i++) {
        var = ABS(clause[tofix][i]);
        numbreak = breakcount[var];
        if (numbreak<=bestvalue) {
            if (numbreak<bestvalue) numbest=0;
            bestvalue = numbreak;
            best[numbest++] = var;
        }
    }
    if (bestvalue>0 && (Random()%denominator < numerator))
        return ABS(clause[tofix][Random()%clausesize]);

    if (numbest == 1) return best[0];
    return best[Random()%numbest];
}

int picknovelty(void)
{
    int var, diff, birthdate;
    int youngest, youngest_birthdate, best, second_best, best_diff, second_best_diff;
    int tofix, clausesize, i;

    tofix = fals[Random()%numfalse];
    clausesize = size[tofix];

    if (clausesize == 1) return ABS(clause[tofix][0]);

    youngest_birthdate = -1;
    best_diff = -BIG;
    second_best_diff = -BIG;

    for(i = 0; i < clausesize; i++) {
        var = ABS(clause[tofix][i]);
        diff = makecount[var] - breakcount[var];
        birthdate = changed[var];
        if (birthdate > youngest_birthdate) {
            youngest_birthdate = birthdate;
            youngest = var;
        }
        if (diff > best_diff || (diff == best_diff && changed[var] < changed[best])) {
            /* found new best, demote best to 2nd best */
            second_best = best;
            second_best_diff = best_diff;
            best = var;
            best_diff = diff;
        }
        else if (diff > second_best_diff || (diff == second_best_diff && changed[var] < changed[second_best])) {
            /* found new second best */
            second_best = var;
            second_best_diff = diff;
        }
    }
    if (best != youngest) return best;
    if ((Random()%denominator < numerator)) return second_best;
    return best;
}

int pickrnovelty(void)
{
    int var, diff, birthdate;
    int diffdiff;
    int youngest, youngest_birthdate, best, second_best, best_diff, second_best_diff;
    int tofix, clausesize, i;

    tofix = fals[Random()%numfalse];
    clausesize = size[tofix];

    if (clausesize == 1) return ABS(clause[tofix][0]);
    if ((numflip % 100) == 0) return ABS(clause[tofix][Random()%clausesize]);

    youngest_birthdate = -1;
    best_diff = -BIG;
    second_best_diff = -BIG;

    for(i = 0; i < clausesize; i++) {
        var = ABS(clause[tofix][i]);
        diff = makecount[var] - breakcount[var];
        birthdate = changed[var];
        if (birthdate > youngest_birthdate) {
            youngest_birthdate = birthdate;
            youngest = var;
        }
        if (diff > best_diff || (diff == best_diff && changed[var] < changed[best])) {
            /* found new best, demote best to 2nd best */
            second_best = best;
            second_best_diff = best_diff;
            best = var;
            best_diff = diff;
        }
        else if (diff > second_best_diff || (diff == second_best_diff && changed[var] < changed[second_best])) {
            /* found new second best */
            second_best = var;
            second_best_diff = diff;
        }
    }
    if (best != youngest) return best;

    diffdiff = best_diff - second_best_diff;

    if (numerator < 50 && diffdiff > 1) return best;
    if (numerator < 50 && diffdiff == 1) {
        if ((Random()%denominator) < 2*numerator) return second_best;
        return best;
    }
    if (diffdiff == 1) return second_best;
    if ((Random()%denominator) < 2*(numerator-50)) return second_best;
    return best;
}


int picktabu(void)
{
    int numbreak[MAXLENGTH];
    int tofix;
    int clausesize;
    int i;			/* a loop counter */
    int best[MAXLENGTH];	/* best possibility so far */
    int numbest;		/* how many are tied for best */
    int bestvalue;		/* best value so far */
    int noisypick;

    tofix = fals[Random()%numfalse];
    clausesize = size[tofix];
    for(i = 0; i < clausesize; i++)
        numbreak[i] = breakcount[ABS(clause[tofix][i])];

    numbest = 0;
    bestvalue = BIG;

    noisypick = (numerator > 0 && Random()%denominator < numerator);
    for (i=0; i < clausesize; i++) {
        if (numbreak[i] == 0) {
            if (bestvalue > 0) {
                bestvalue = 0;
                numbest = 0;
            }
            best[numbest++] = i;
        }
        else if (tabu_length < numflip - changed[ABS(clause[tofix][i])]) {
            if (noisypick && bestvalue > 0) {
                best[numbest++] = i;
            }
            else {
                if (numbreak[i] < bestvalue) {
                    bestvalue = numbreak[i];
                    numbest = 0;
                }
                if (numbreak[i] == bestvalue) {
                    best[numbest++] = i;
                }
            }
        }
    }
    if (numbest == 0) return NOVALUE;
    if (numbest == 1) return Var(tofix, best[0]);
    return (Var(tofix, best[Random()%numbest]));
}

int countunsat(void)
{
    int i, j, unsat, bad, lit, sign;

    unsat = 0;
    for (i=0; i < numclause; i++)
    {
        bad = TRUE;
        for (j=0; j < size[i]; j++)
        {
            lit = clause[i][j];
            sign = lit > 0 ? 1 : 0;
            if ( atom[ABS(lit)] == sign )
            {
                bad = FALSE;
                break;
            }
        }
        if (bad)
            unsat++;
    }
    return unsat;
}


#ifdef NT
double elapsed_seconds(void) {
    return 0.0;
}
#else
double elapsed_seconds(void)
{
    double answer;

    static struct tms prog_tms;
    static long prev_times = 0;

    (void) times(&prog_tms);

    answer = ((double)(((long)prog_tms.tms_utime)-prev_times))/((double) CLK_TCK);

    prev_times = (long) prog_tms.tms_utime;

    return answer;
}
#endif

void print_sol_cnf(std::vector<bool> & sol)
{
    std::ofstream outfile;
    if (!bsp::run_context()) outfile.open("outfile", std::ios::app);
    int i;
    sol.resize(numatom);
    for(i = 1; i < numatom+1; i++) {
        //       bsp::console_printf("v %i\n", solution[i] == 1 ? i : -i);
        outfile <<(solution[i] == 1 ? true : false)  <<" ";
        sol[i-1]=solution[i] == 1 ? true : false;
    }
    outfile<<std::endl;
}

void
print_low_assign(long int lowbad)
{
    int i;

    bsp::console_printf("Begin assign with lowest # bad = %li\n", lowbad);
    for (i=1; i<=numatom; i++) {
        bsp::console_printf(" %d", lowatom[i]==0 ? -i : i);
        if (i % 10 == 0) bsp::console_printf("\n");
    }
    if ((i-1) % 10 != 0) bsp::console_printf("\n");
    bsp::console_printf("End assign\n");
}

void
print_current_assign(void)
{
    int i;

    bsp::console_printf("Begin assign at flip = %li\n", numflip);
    for (i=1; i<=numatom; i++) {
        bsp::console_printf(" %d", atom[i]==0 ? -i : i);
        if (i % 10 == 0) bsp::console_printf("\n");
    }
    if ((i-1) % 10 != 0) bsp::console_printf("\n");
    bsp::console_printf("End assign\n");
}

void
save_low_assign(void)
{
    int i;

    for (i=1; i<=numatom; i++)
        lowatom[i] = atom[i];
}

void
save_solution(void)
{
    int i;

    for (i=1; i<=numatom; i++)
        solution[i] = atom[i];
}




int WalkSatMemory(std::vector<std::vector<bool>> &sol, unsigned n, const std::vector<std::vector<int>> &clauses) {
    if (clauses.empty()) { sol.push_back(std::vector<bool>(n,false)); return 0; }
    memory_clauses=&clauses; memory_n=n;
    struct Reset { ~Reset() { memory_clauses=nullptr; } } reset;
    std::vector<std::string> args={"walksat","-solcnf","-cutoff","100000000","-best","-seed","1234567","-numsol","10"};
    std::vector<char*> argv; for (auto &arg:args) argv.push_back(&arg[0]);
    return WalkSat(sol,argv.size(),argv.data());
}
