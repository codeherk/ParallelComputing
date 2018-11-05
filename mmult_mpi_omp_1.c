#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))
//Saved Chau
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
  //Var to use for matrix
  int i, j, n;
  double *buffer, *data_received, *result;
  int sender;
  int anstype;
  int row_counter; //counter of number of rows sent
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  FILE *file1;
	FILE * file2;
        char line[100];
        int row1, col1, row2, col2;
 

  //Arg must more than 2
  if (argc > 2) {
    //nrows = atoi(argv[1]);
    //ncols = nrows;
    if (myid == 0) {
      // Master Code goes here

	//Here is what I have for reading the size of 2 array
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
	printf("\nRow1: %d by Column1 %d\n", row1,col1);

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
	
	
	//Get matrix A
	aa=(double*)malloc(sizeof(double) * row1 * col1);
        for(i=0;i<row1;i++)
        	for(j=0;j<col1;j++)
        		n= fscanf(file1,"%lf",&aa[i*col1+j]);
	//Get matrix B
	bb=(double*)malloc(sizeof(double) * row2 * col2);
		for(i=0;i<row2;i++)
			for(j=0;j<col2;j++)
		      		n= fscanf(file2,"%lf",&bb[i*col2+j]);


    	/*Here is the test to print out 2 matrix*/
    	printf("Here is the test\n");
   	printf("print A\n");
	for(i=0;i<row1;i++)
        {
         for(j=0;j<col1;j++)
                {
                printf("%lf   ",aa[i*col1+j]);
                }
                printf("\n");
        }
    printf("print B\n");

        for(i=0;i<row2;i++)
        {
         for(j=0;j<col2;j++)
                {
                printf("%lf   ",bb[i*col2+j]);
                }
                printf("\n");
        }
		
	//Here is the master code to send to the slave	    
      //aa = gen_matrix(nrows, ncols);
      //bb = gen_matrix(ncols, nrows);
     // printf("%f",*aa);
	//cc1 is the result matrix
	//get the size of matrix c 
	nrows=row1;
	ncols=col2;
      cc1 = malloc(sizeof(double) * nrows * nrows);      
	 starttime = MPI_Wtime();
	
	row_counter=0;//initial the counter for number of the row
      /* Insert your master code here to store the product into cc1 */
	//send broadast matrix B to slave 
	//MPI_Bcast(buffer, count, datatyoe, o, MPI_COMM_WORLD);
	MPI_Bcast(bb,col1*col2,MPI_DOUBLE, 0, MPI_COMM_WORLD);
	//send a row of matrix
	for(i =0; i<min (numprocs-1,nrows);i++){
		for(int j =0; j<ncols;j++){
		  buffer[j]=aa[i*col1+j];	
		}
		MPI_Send(buffer,row1, MPI_DOUBLE, i+1,i+1,MPI_COMM_WORLD);
		row_counter++;
	}
	//Receive  data from slave
	double *data_received=(double *) malloc(sizeof(double) * ncols);
	for(i=0; i<nrows;i++){
		MPI_Recv(data_received, ncols,MPI_DOUBLE, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		sender=status.MPI_SOURCE;
		anstype=status.MPI_TAG;
		//fill cc1 with data receiced
		for(j=0;j<ncols;j++){
			cc1[(anstype-1)*ncols+j]=data_received[j];
		}
	}
	
	
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, row1, col1, bb, row2, col2);
      compare_matrices(cc2, cc1, nrows, nrows);

	
    } else {
      // Slave Code goes here
 
 	bb=(double*) malloc(sizeof(double)*col1*col2);
	MPI_Bcast(bb,col1*col2,MPI_DOUBLE,0,MPI_COMM_WORLD);
	
	//check to see if there rows to process
	if(myid<=nrows){
		while(1){
			MPI_Recv(buffer,col1,MPI_DOUBLE,0,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if(status.MPI_TAG==0){
				break;
			}
		//find the current row from the master
			row_counter=status.MPI_TAG;
			result=(double *)malloc(sizeof(double)*ncols);
			
			for(i=0;i<ncols;i++){
				for(int j=0; j<col1; j++){
					result[i]+= buffer[j]*bb[j*ncols+i];
				}
			}
		MPI_Send(result,ncols,MPI_DOUBLE,0,row_counter,MPI_COMM_WORLD);
		}
	}   
}
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
