#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "mpi.h"
#include <time.h>
#define min(x, y) ((x)<(y)?(x):(y))

/*
I basically reuse the mpi and omp matrix times vector code by sending each column of
the second matrix as the column "b" and repeat process for number of columns of matrix 2. 
*/

int main(int argc, char* argv[])
{
  int i,j,iter;
  int multipliable; //boolean to see if can matrix multipy
  int nrows, ncols,*dimen;
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
if(argc>2)
{
 //Master will parse throught texts files and dynmatically create arrays.
   if(myid==0)
{
        FILE * file1; //File Pointer for text 1
        FILE * file2; //File Pointer for text 2
        int row1,col1,row2,col2;
        double *matrix1,*matrix2;

        file1=fopen(argv[1],"r");
        if( file1 == NULL)
        {
        printf("\nError Opening File of File does not exist\n");
//Sends a message to slaves to terminate 
for(i=0;i<numprocs-1;i++){
          MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
        exit(0);
        }
        char line[128];
        //Gets row and col values for first file
        if(fgets(line,sizeof line, file1)!=NULL)
        {
        //Gets first line and parses to get row value
         char *ret;
         ret=strstr(line,"rows(");
         char *value;
         value= strchr(ret,')');
         int index;
         index= (int)(value-ret);
         value=strndup(ret+5,index-5);
         row1=atoi(value);
        //Gets column value  
         ret=strstr(ret,"cols(");
         value=strchr(ret,')');
         index=(int)(value-ret);
         value= strndup(ret+5,index-5);
         col1=atoi(value);
        }
       // printf("%d rows,%d column\n",row1,col1);

 //Allocate Memory & Populate Matrix 1
         matrix1=(double*)malloc(sizeof(double) * row1 * col1);
        for(i=0;i<row1;i++)
        {
         for(j=0;j<col1;j++)
                {
               int hold= fscanf(file1,"%lf",&matrix1[i*col1+j]);
                }
        }
        fclose(file1);

        //Open File 2
        file2=fopen(argv[2],"r");
        if( file2 == NULL)
        {
                printf("\nError Opening File or File does not exist\n");
               //Sends slave message to exit 
for(i=0;i<numprocs-1;i++){
          MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
 exit(0);
        }
        //Gets row and col values for second file
        if(fgets(line,sizeof line, file2)!=NULL)
        {
        //Gets first line and parses to get row value
         char *ret=strstr(line,"rows(");
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
        //printf("%d rows,%d column\n",row2,col2);
        
        //Allocate Memory & Populate Matrix 2
         matrix2=(double*)malloc(sizeof(double) * row2 * col2);
        for(i=0;i<row2;i++)
        {
         for(j=0;j<col2;j++)
                {
                int hold=fscanf(file2,"%lf",&matrix2[i*col2+j]);
                }
        }
        fclose(file2);


        //Print 2 matrixs
        printf("Matrix 1\n");
        printf("%d rows,%d columns\n",row1,col1);
        for(i=0;i<row1;i++){
          for(j=0;j<col1;j++){ 
            printf("%f ",matrix1[i*col1+j]);
        }
        printf("\n");
        }
        printf("Matrix2\n");
        printf("%d rows,%d columns\n",row2,col2);
        for(i=0;i<row2;i++){
          for(j=0;j<col2;j++){ 
            printf("%f ",matrix2[i*col2+j]);
        }
        printf("\n");
        }

        if(col1==row2)
                multipliable=1;
        else
                multipliable=0;

        //ACTUALYY MULTIPY and start distributing rows and columns
        if(multipliable==1)
        {
        //printf("\nWe can multiply the matrices");
        ncols=col1;
        nrows=row1;
        dimen[0]=nrows;
        dimen[1]=ncols;
        dimen[2]=col2;
        //Sends each slave the neccesary info like row and colmns
        for(i=0;i<numprocs-1;i++){
        MPI_Send(dimen, 3, MPI_INT, i+1, i+1, MPI_COMM_WORLD);
        //printf("\nSENDING NOTFICATION CAN MULTIPY TO SLAVE");
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
        starttime= MPI_Wtime();
        for(iter=0;iter<col2;iter++)
         {
                /*
                Take each column of second matrix to be b vector and rest is just 
                matrix times vector code
                */
                for( i=0;i<ncols;i++)
                {
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
                        //printf("\nRECIEVED ANS %f",ans);
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
      //printf("\nReached end with time %f",(endtime - starttime));

        //print the product matrix
        printf("Result matrix is stored in result.txt\n");
        for(i=0;i<row1;i++){
          for(j=0;j<col2;j++){ 
            printf("%f ",c[i][j]);
        }
        printf("\n");
        }
        //store the product result in text file
        FILE * fp3; // File pointer to produce result text file 
        fp3=fopen("result.txt","w");
        fprintf(fp3,"rows(%d) cols(%d)\n",row1,col2);

        //printf("\n\nProduct of two matrices in result.txt\n");
        for(i=0;i<row1;i++)
        {
        for(j=0;j<col2;j++)
        { fprintf(fp3,"%f ",c[i][j]);
        }
        fprintf(fp3,"\n");
        }
        fclose(fp3);
     }
    //If dimensions not right print out cant multiply and notify all slaves to end 
       else{
        printf("\nDimensions do no allow multiplication\n");
        for(i=0;i<numprocs-1;i++){
          MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
        }

} else {
 /* SLAVE CODE
 Will recieve certain info on dimenionsion of matrixs and will learn 
 if can actually multiply or just stop slaves
 */
 MPI_Recv(dimen, 3, MPI_INT, 0, MPI_ANY_TAG,
                   MPI_COMM_WORLD, &status);
      if(status.MPI_TAG==0){
        multipliable==0;
        }
    //If can multiply allocate memory in slave
      else{
        multipliable=1;
        nrows=dimen[0];
        ncols=dimen[1];
        iter=dimen[2];
        //printf("\\nRECIEDCE DIMEN %d %d %d",nrows,ncols,iter);
        b = (double*)malloc(sizeof(double) * ncols);
        buffer = (double*)malloc(sizeof(double) * ncols);
        }
    //If can multiply actually multiply by getting a new column each iteration
     if(multipliable==1){
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

//If not enought arguments print out correct formating
else{
if(myid==0){
fprintf(stderr, "Usage mpiexec -f ~/hosts -n 2 ./mmult_mpi_omp <name1.txt> <name2.txt>\n");
}}
MPI_Finalize();
return 0;

}

