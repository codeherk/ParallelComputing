#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#define min(x, y) ((x)<(y)?(x):(y))
#define KCYN  "\x1B[36m" //CYAN color
#define RESET "\x1B[0m"
/**
  Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
**/

int main(int argc, char* argv[]){
  int i,j,iter;
  int canMultiply; // flag
  int nrows, ncols,*dimen; // 
  dimen=(int*)malloc(sizeof(int) * 3);
  double  *b, **c;
  double *buffer, ans;
  int run_index;
  int nruns;
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  int  numsent, sender;
  int anstype, row;
  srand(time(0));
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  
  //Checks to see if arguments greater then 2 
  if(argc>2){
    //Master will parse throught texts files and dynmatically create arrays.
    if(myid==0){

        FILE * file1; 
        FILE * file2;
        int row1,col1,row2,col2;
        double *matrix1,*matrix2;

        file1=fopen(argv[1],"r"); // open file provided in argument 1

        // exit if file 1 does not exit
        if( file1 == NULL){
          printf("\nError opening file, file does not exist\n");
          //Sends a message to slaves to terminate 
          for(i=0;i<numprocs-1;i++){
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
          exit(0);
        }

        char line[128];

        // gets row and col values for first file
        if(fgets(line, sizeof line, file1)!=NULL){
          // gets first line and parses to get row value
          // Code commented out below because its too many lines of code when it can be done in 4 (see above)
          char *ret;
          ret = strstr(line,"rows(");
          char *value;
          value = strchr(ret,')');
          int index;
          index = (int)(value - ret);
          value =strndup(ret+5,index-5);
          row1 = atoi(value);

          //Gets column value  
          ret=strstr(ret,"cols(");
          value=strchr(ret,')');
          index=(int)(value-ret);
          value= strndup(ret+5,index-5);
          col1=atoi(value); 

        }
        
        // allocate memory & populate Matrix from file 1
        matrix1 = (double*)malloc(sizeof(double) * row1 * col1); 
        for(i=0; i < row1; i++){
          for(j=0; j < col1; j++){
            
            // no need to return fscanf to hold. this variable was only used twice
            //int hold = fscanf(file1,"%lf",&matrix1[i*col1+j]);
            fscanf(file1, "%lf" , &matrix1[i * col1 + j]);
          }
        }
        // close file after reading
        fclose(file1); 

        // open File 2
        file2 = fopen(argv[2],"r");
        //exit if file does not exist
        if(file2 == NULL){
          printf("\nError opening file, file does not exist\n");
          //Sends slave message to exit 
          for(i = 0; i < numprocs - 1; i++){
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
          exit(0);
        }

        // gets row and column values for second file
        if(fgets(line,sizeof line, file2)!=NULL){
          
          //Gets first line and parses to get row value
          char *ret = strstr(line,"rows("); 
          char *value= strchr(ret,')');
          int index = (int)(value-ret);
          value=strndup(ret+5,index-5);
          row2=atoi(value);

          //Gets column value  
          ret=strstr(ret,"cols(");
          value=strchr(ret,')');
          index=(int)(value-ret);
          value= strndup(ret+5,index-5);
          col2=atoi(value);
        }

        
        // allocate memory & populate Matrix from file 2
        matrix2=(double*)malloc(sizeof(double) * row2 * col2);
        for(i=0;i<row2;i++) {
         	for(j=0;j<col2;j++){
             //int hold=fscanf(file2,"%lf",&matrix2[i*col2+j]);
             fscanf(file2, "%lf", &matrix2[i * col2 + j]);
          }
        }
        //close file 2
        fclose(file2);


        // print the 2 matrices
        printf("Matrix from %s\n", argv[1]);
        printf("%d rows,%d columns\n",row1,col1);
        for(i=0;i<row1;i++){
          for(j=0;j<col1;j++){ 
            printf(KCYN "%f " RESET,matrix1[i*col1+j]);
	  }
	  printf("\n");
        }
        printf("\nMatrix from %s\n", argv[2]);
        printf("%d rows,%d columns\n",row2,col2);
        for(i=0;i<row2;i++){
          for(j=0;j<col2;j++){ 
            printf(KCYN "%f " RESET,matrix2[i*col2+j]);
       	  }
	  printf("\n");
        }

        if(col1 == row2)
                canMultiply = 1;
        else
                canMultiply = 0;

        //Check to see if you can multiply the matrices
        if(canMultiply == 1){
          ncols = col1;
          nrows = row1;
          dimen[0] = nrows;
          dimen[1] = ncols;
          dimen[2] = col2;

          //Sends each slave the neccesary info like row and colmns
          for(i=0;i<numprocs-1;i++){
            MPI_Send(dimen, 3, MPI_INT, i+1, i+1, MPI_COMM_WORLD);
          }
          b = (double*)malloc(sizeof(double) * ncols);
          c = (double **)malloc(row1 * sizeof(double *));
          for (i=0; i<row1; i++)
            c[i] = (double *)malloc(col2 * sizeof(double));
          buffer = (double*)malloc(sizeof(double) * ncols);
          /*
          We will use the matrix times vector as a guideline as we will 
          take each column of second matrix to be our vector and keep sending a new column
          */
          starttime = MPI_Wtime();
          for(iter=0; iter<col2; iter++) {
            /*
            Take each column of second matrix to be b vector and rest is just 
            matrix times vector code
            */
            for( i=0; i < ncols; i++){
              b[i]=matrix2[i*col2+iter];
            }

            numsent = 0;
            MPI_Bcast(b, ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            for (i = 0; i < min(numprocs-1, nrows); i++) {
              for (j = 0; j < ncols; j++) {
                buffer[j] = matrix1[i * ncols + j];
              }
              MPI_Send(buffer, ncols, MPI_DOUBLE, i+1, i+1, MPI_COMM_WORLD);
              numsent++;
            }

            for (i = 0; i < nrows; i++) {
              MPI_Recv(&ans, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
                      MPI_COMM_WORLD, &status);
              sender = status.MPI_SOURCE;
              anstype = status.MPI_TAG;
              c[anstype-1][iter] = ans;
              
              if (numsent < nrows) {
                for (j = 0; j < ncols; j++) {
                  buffer[j] = matrix1[numsent*ncols + j];
                }
                MPI_Send(buffer, ncols, MPI_DOUBLE, sender, numsent+1,
                          MPI_COMM_WORLD);
                numsent++;
              }else {
                MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, sender, 0, MPI_COMM_WORLD);
              }

            }
          }

          endtime= MPI_Wtime();

          //print the product matrix
          printf("\nResult matrix is stored in result.txt\n");
          for(i=0;i<row1;i++){
            for(j=0;j<col2;j++){ 
              printf(KCYN"%f " RESET,c[i][j]);
            }
            printf("\n");
          }

          //store the result in another text file
          FILE * file3; 
          file3 = fopen("result.txt","w");
          fprintf(file3,"rows(%d) cols(%d)\n",row1,col2);


          for(i=0;i<row1;i++) {
            for(j=0;j<col2;j++){ 
              fprintf(file3,"%f ",c[i][j]);
            }
            fprintf(file3,"\n");
          }
          fclose(file3);
        }

        //If dimensions are not correct, you can't multiply the matrices. Notified all slaves to end 
        else{
          printf("\nDimensions do not allow multiplication\n");
          for(i=0;i<numprocs-1;i++){
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
        }
      }else {

        /* SLAVE CODE
        Will receive certain info on dimensions of matrices and will determine whether they can be calculated or not
        */

        MPI_Recv(dimen, 3, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if(status.MPI_TAG==0){
          canMultiply==0;
        }

        // Allocate memory based on dimensions
        else{
          canMultiply=1;
          nrows=dimen[0];
          ncols=dimen[1];
          iter=dimen[2];
          
          b = (double*)malloc(sizeof(double) * ncols);
          buffer = (double*)malloc(sizeof(double) * ncols);
        }

        // Getting a new column each iteration
        if(canMultiply==1){
          for(i=0;i<iter;i++){
            MPI_Bcast(b, ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            if (myid <= nrows) {
              while(1) {
                MPI_Recv(buffer, ncols, MPI_DOUBLE, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, &status);
                if (status.MPI_TAG == 0){
                  break;
                }
                row = status.MPI_TAG;
                ans = 0.0;
                for (j = 0; j < ncols; j++) {
                  ans += buffer[j] * b[j];
                }
                MPI_Send(&ans, 1, MPI_DOUBLE, 0, row, MPI_COMM_WORLD);
              }
            }
          }
        }
      }
    }//end of if arg greater that 2
    // If not enought arguments print out correct formatting
    else{
      if(myid==0){
	      fprintf(stderr, "Usage mpiexec -f ~/hosts -n 2 ./mmult_mpi_omp <name1.txt> <name2.txt>\n");
	    }
    }
    
    MPI_Finalize();
    return 0;

}

