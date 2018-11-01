#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* gen_matrix(int n, int m);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
*/

int main(int argc, char* argv[])
{
  int nrows, ncols;
  double *aa;	/* the A matrix */
  double *bb;	/* the B matrix */
  double *cc1;	/* A x B computed using the omp-mpi code you write */
  double *cc2;	/* A x B computed using the conventional algorithm */
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);



  //Arg must more than 2
  if (argc > 2) {
    //nrows = atoi(argv[1]);
    //ncols = nrows;
    if (myid == 0) {
      // Master Code goes here

	//Here is what I have for reading the size of 2 array
	FILE * file1;
	FILE * file2;
	char line[100];
	int row1, col1, row2, col2;
	file1=fopen(argv[1],"r");
	file2=fopen(argv[2],"r");
	//If one of the file is NULL, exit
	//However, I beleive wee need to send a message to the slaves to terminate
	//Have to do with MPI_Send
	if(file1==NULL||file2==NULL){
		printf("Fopen failed\n");
		return 1;
	}
	//open the first file and get the number of row and collumn for first file
	if(fgets(line, sizeof line, file1)){
		//get row
		char *token = strtok(line, "rows()cols ");
		row1=atoi(token);
		//get col
		token = strtok(NULL, "rows()cols ");
		col1 =atoi(token);
	}
	printf("\nRow: %d by Column %d\n", row1,col1);
	fclose(file1);

	//Read the file 2 and get row and col of second matrix 
	if(fgets(line, sizeof line, file2)){
                //get row2
                char *token = strtok(line, "rows()cols ");
                row2=atoi(token);
                //get col2
                token = strtok(NULL, "rows()cols ");
                col2 =atoi(token);
        }
        printf("\nRow2 : %d by Column2 %d\n", row2,col2);
	fclose(file2);

      //aa = gen_matrix(nrows, ncols);
      //bb = gen_matrix(ncols, nrows);
     // printf("%f",*aa);
      //cc1 = malloc(sizeof(double) * nrows * nrows); 
      starttime = MPI_Wtime();
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);
    } else {
      // Slave Code goes here
    }
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
