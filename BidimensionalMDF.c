#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>

/**********Para calcular tiempo*************************************/
double dwalltime()
{
        double sec;
        struct timeval tv;

        gettimeofday(&tv,NULL);
        sec = tv.tv_sec + tv.tv_usec/1000000.0;
        return sec;
}
/****************************************************************/

//TODO: optimizar inicialización de la matriz (j primero, creo?)
//Inicializa una matriz
float* inicializarMatriz(float* m,unsigned long N){
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            m[i*N+j]=((float)rand())/(float)RAND_MAX;
            //printf("m[%i,%i]: %.2f\n", i, j, m[i*N+j]);
        }
    }
    return m;
}

int main(int argc, char* argv[]){

    //Se comprueba que se hayan pasado la cantidad correcta de parámetros de entrada. Evita que tire un coredump en caso de que se corra mal el script. 
    if ((argc != 3) || (atoi(argv[1]) <= 0) || (atoi(argv[1]) <= 0)){
        printf("\nUsar: %s N T\n N: Dimension del vector\n T: Cantidad de procesos\n", argv[0]);
        exit(1);
    }

    double timetick, timetickCom, tComunicacion = 0;

    float* swap;
    float* mSec;
    float* mSecConvergido;
    float* mPar;
    float* mParConvergido;

    int i, j, iteraciones = 0;
    bool convergio = false;
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N*N;
    unsigned char P = atol(argv[2]);

    int myrank;
    MPI_Status status;
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    float unCuarto = 1./4.0;
    float unSexto = 1./6.0;
    float unNoveno = 1./9.0;

    if(myrank == 0){

        printf("Se ejecuta el algoritmo de convergencia para N = %i P = %i\n",N,P);
        printf("----------------------------------------------------------\n");

        //Se inicializan las matrices
        mSec=(float*)malloc(numBytes);
        mSecConvergido=(float*)malloc(numBytes);
        mPar=(float*)malloc(numBytes);
        mParConvergido=(float*)malloc(numBytes);

        //Se inicializan las matrices
        mSec=inicializarMatriz(mSec,N);
        //Para poder hacer una comparación apropiada, ambas estrategias van a trabajar sobre el mismo set de datos
        for(i=0;i<N;i++){
            for(j=0;j<N;j++){
                mPar[i*N+j]=mSec[i*N+j];
            }
        }

        timetick = dwalltime();
        //Se almacenan los resultados de estas operaciones para no tener que recalcularlos constantemente y para reemplazar las divisiones con productos
        register float valorAComparar;

        //Primero se resuelve el problema con la estrategia secuencial
        while(!convergio){
            iteraciones++;

            //La posicion de un valor de la matriz se calcula como i*N+j

            //Primero se calcula el valor de las esquinas. Notese que el indice maximo para cada variable es N-1, no N.
            //Esquina superior izquierda (0,0).
            mSecConvergido[0] = ((
                mSec[0] +
                mSec[1] +
                mSec[N] +
                mSec[N+1]) * unCuarto);
            
            //Esquina superior derecha (0,N-1).
            mSecConvergido[N-1] = ((
                mSec[N-1] + 
                mSec[N-2] + 
                mSec[(2*N)-1] + 
                mSec[(2*N)-2]) * unCuarto);

            //Esquina inferior izquierda (N-1,0).
            mSecConvergido[(N-1)*N] = ((
                mSec[(N-1)*N] + 
                mSec[(N-1)*N+1] + 
                mSec[(N-2)*N] + 
                mSec[(N-2)*N+1]) * unCuarto);

            //Esquina inferior derecha (N-1,N-1)
            mSecConvergido[N*N -1] = ((
                mSec[N*N -1] + 
                mSec[N*N -2] + 
                mSec[(N-1)*N -1] + 
                mSec[(N-1)*N -2]) * unCuarto);

            //Luego se calcula el valor de los bordes, menos las esquinas.
            for(j=1;j<N-1;j++){
                mSecConvergido[j]=(
                    mSec[j] +
                    mSec[j-1] +
                    mSec[j+1] +
                    mSec[j+N] +
                    mSec[j+N+1] +
                    mSec[j+N-1]) * unSexto;

                mSecConvergido[N*(N-1)+j]=(
                    mSec[N*(N-1)+j] +
                    mSec[N*(N-1)+j-1] +
                    mSec[N*(N-1)+j+1] +
                    mSec[N*(N-2)+j] +
                    mSec[N*(N-2)+j+1] +
                    mSec[N*(N-2)+j-1]) * unSexto;    
            }

            for(i=1;i<N-1;i++){
                mSecConvergido[i*N] = (
                    mSec[i*N] +
                    mSec[i*N+1] +
                    mSec[(i-1)*N] +
                    mSec[(i-1)*N+1] +
                    mSec[(i+1)*N] +
                    mSec[(i+1)*N+1]) * unSexto;

                mSecConvergido[i*N+(N-1)] = (
                    mSec[i*N+(N-1)] +
                    mSec[i*N+(N-1)-1] +
                    mSec[(i-1)*N+(N-1)] +
                    mSec[(i-1)*N+(N-1)-1] +
                    mSec[(i+1)*N+(N-1)] +
                    mSec[(i+1)*N+(N-1)-1]) * unSexto;
            }

            //Luego se calcula el promedio de la matriz interna, para 1<i<N-1, 1<j<N-1
            for(i=1;i<N-1;i++){
                for(j=1;j<N-1;j++){
                    mSecConvergido[i*N+j]=(
                        mSec[(i-1)*N+j-1] +
                        mSec[(i)  *N+j-1] +
                        mSec[(i+1)*N+j-1] +
                        mSec[(i-1)  *N+j] +
                        mSec[(i)    *N+j] +
                        mSec[(i+1)  *N+j] +
                        mSec[(i-1)*N+j+1] +
                        mSec[(i)  *N+j+1] +
                        mSec[(i+1)*N+j+1]
                    ) * unNoveno ;
                }
            }

            //Se evalúa si todos los valores del vector convergieron.
            convergio = true;
            valorAComparar = mSecConvergido[0];
            for(int i=1;i<N*N & convergio;i++){
                //Si algún valor no estuviera por debajo del límite de precisión, se copia el valor del vector actual al original
                if(!(fabs(valorAComparar - mSecConvergido[i])<0.01)){
                    convergio = false;
                    swap = mSec;
                    mSec = mSecConvergido;
                    mSecConvergido = swap;
                }
            }
        }
        printf("Usando la estrategia secuencial en dos dimensiones, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", mSecConvergido[0], iteraciones, dwalltime() - timetick);
    }

    //PROCESAMIENTO PARALELO CON MPI

    convergio = false;
    float *subMatrizPar;
    float *mResP;
    float valorAComparar; //contiene el valor de convergencia

    //Variables usadas por cada proceso que contienen el elemente anterior o siguiente de su primera o ultima posicion, respectivamente.
    //Usadas para calcular el promedio de sus valores bordes.
    float *vBordeSuperior, *vBordeInferior;
    
    if(myrank != 0)
        vBordeSuperior = (float*)malloc(sizeof(float)*N);
    if(myrank != P-1)
        vBordeInferior = (float*)malloc(sizeof(float)*N);
    
    //Cada proceso tendra N/P filas de N elementos cada una
    subMatrizPar = (float*)malloc(sizeof(float)*(N/P)*N);
    mResP = (float*)malloc(sizeof(float)*(N/P)*N); 

    iteraciones = 0;
    bool convergioLocal = true; //variable de convergencia local que sera reducida
    //Barrera para asegurarse de que todos los procesos esperen a que 0 termine de ejecutar el algoritmo secuencial
    MPI_Barrier(MPI_COMM_WORLD);

    if(myrank == 0){
        timetickCom = dwalltime();
    }
    timetick = dwalltime();  //Para MPI tenemos en cuenta el overhead de comunicacion que arranca ahora
    MPI_Scatter(mPar,(N/P)*N,MPI_FLOAT,subMatrizPar,(N/P)*N,MPI_FLOAT,0,MPI_COMM_WORLD); //cada proceso recibe una parte del Matriz original
    if(myrank == 0){
        tComunicacion = tComunicacion + (dwalltime() - timetickCom);
    }

    while(!convergio){
        iteraciones++;

        
        //Cada proceso le manda su ultima fila al siguiente, en orden para asegurarse que no ocurran deadlocks
        if(myrank == 0){
            timetickCom = dwalltime();
        }
        
        if(myrank == 0){
            MPI_Send(&subMatrizPar[N*((N/P)-1)], N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD);
        }
        
        if(myrank > 0 && myrank < P-1){
            MPI_Recv(vBordeSuperior,N, MPI_FLOAT, myrank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&subMatrizPar[N*((N/P)-1)], N, MPI_FLOAT, myrank+1, 0, MPI_COMM_WORLD);
        }
        
        if(myrank == P-1){
            MPI_Recv(vBordeSuperior, N, MPI_FLOAT, P-2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        //Ahora que se tienen todos los valores del borde inferior, se repite la cadena de mensajes pero en sentido inverso
        if(myrank == P-1){
            MPI_Send(&subMatrizPar[0], N, MPI_FLOAT, P-2, 0, MPI_COMM_WORLD);
        }
        if(myrank > 0 && myrank < P-1){
            MPI_Recv(vBordeInferior, N, MPI_FLOAT, myrank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&subMatrizPar[0], N, MPI_FLOAT, myrank-1, 0, MPI_COMM_WORLD);
        }
        if(myrank == 0){
            MPI_Recv(vBordeInferior, N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }

        //Se procesa de distinta manera, de acuerdo a si el elemento es una esquina, borde o esta en el medio
        if(myrank == 0){
            //Esquina superior izquierda (0,0).
            mResP[0] = ((
                subMatrizPar[0] +
                subMatrizPar[1] +
                subMatrizPar[N] +
                subMatrizPar[N+1]) * unCuarto);
            
            //Esquina superior derecha (0,N-1).
            mResP[N-1] = ((
                subMatrizPar[N-1] + 
                subMatrizPar[N-2] + 
                subMatrizPar[(2*N)-1] + 
                subMatrizPar[(2*N)-2]) * unCuarto);

            //Borde superior
            for(j=1;j<N-1;j++){
                mResP[j] = (
                    subMatrizPar[j-1] +
                    subMatrizPar[j] +
                    subMatrizPar[j+1] +
                    subMatrizPar[N+j-1] +
                    subMatrizPar[N+j] +
                    subMatrizPar[N+j+1]) * unSexto;
            }

        }
        else if(myrank == P-1){
            //Esquina inferior izquierda (N-1,0).
            mResP[((N/P)-1)*N] = ((
                subMatrizPar[((N/P)-1)*N] + 
                subMatrizPar[(((N/P)-1)*N)+1] + 
                subMatrizPar[((N/P)-2)*N]  + 
                subMatrizPar[(((N/P)-2)*N)+1]) * unCuarto);

            //Esquina inferior derecha (N-1,N-1)
            mResP[(N/P)*N -1] = ((
                subMatrizPar[(N/P)*N -1] + 
                subMatrizPar[(N/P)*N -2] + 
                subMatrizPar[(N/P)*N -N -1] + 
                subMatrizPar[(N/P)*N -N -2]) * unCuarto);

            //Borde inferior
            for(j=1;j<N-1;j++){
                mResP[(((N/P)-1)*N) + j] = ((
                    subMatrizPar[(((N/P)-1)*N) + j] +
                    subMatrizPar[(((N/P)-1)*N) + j + 1] +
                    subMatrizPar[(((N/P)-1)*N) + j - 1] +
                    subMatrizPar[(((N/P)-2)*N) + j] +
                    subMatrizPar[(((N/P)-2)*N) + j + 1] +
                    subMatrizPar[(((N/P)-2)*N) + j - 1]) * unSexto);
            }

        }
        
        if(myrank > 0){

            //Borde superior interno
 
            //Esquina superior izquierda (0,0).
            mResP[0] = (
                subMatrizPar[0] +
                subMatrizPar[1] +
                subMatrizPar[N] +
                subMatrizPar[N+1] +
                vBordeSuperior[0] +
                vBordeSuperior[1]) * unSexto;
            
            //Esquina superior derecha (0,N-1).
            mResP[N-1] = (
                subMatrizPar[N-1] + 
                subMatrizPar[N-2] + 
                subMatrizPar[(2*N)-1] + 
                subMatrizPar[(2*N)-2] +
                vBordeSuperior[N-2] +
                vBordeSuperior[N-1]) * unSexto;

            //Borde interno superior
            for(j=1;j<N-1;j++)
                mResP[j]=(vBordeSuperior[j]
                    +vBordeSuperior[j-1]
                    +vBordeSuperior[j+1]
                    +subMatrizPar[j]
                    +subMatrizPar[j+1]
                    +subMatrizPar[j-1]
                    +subMatrizPar[N+j]
                    +subMatrizPar[N+j+1]
                    +subMatrizPar[N+j-1]) * unNoveno;

        }

        if(myrank < P-1){
               
            //Borde inferior interno

            //Esquina inferior izquierda (N-1,0).
            mResP[((N/P)-1)*N] = (
                subMatrizPar[((N/P)-1)*N] + 
                subMatrizPar[(((N/P)-1)*N) + 1] + 
                subMatrizPar[((N/P)-2)*N] + 
                subMatrizPar[(((N/P)-1)*N) + 2] +
                vBordeInferior[0] +
                vBordeInferior[1]) * unSexto;

            //Esquina inferior derecha (N-1,N-1)
            mResP[(N/P)*N -1] = (
                subMatrizPar[(N/P)*N -1] + 
                subMatrizPar[(N/P)*N -2] + 
                subMatrizPar[(N/P)*N -N -1] + 
                subMatrizPar[(N/P)*N -N -1] +
                vBordeInferior[N-1] +
                vBordeInferior[N-2]) * unSexto;

            //Borde interno inferior
            for(j=1;j<N-1;j++){
                mResP[(((N/P)-1)*N) + j] = (
                    subMatrizPar[(((N/P)-1)*N) + j] +
                    subMatrizPar[(((N/P)-1)*N) + j + 1] +
                    subMatrizPar[(((N/P)-1)*N) + j - 1] +
                    subMatrizPar[(((N/P)-2)*N) + j] +
                    subMatrizPar[(((N/P)-2)*N) + j + 1] +
                    subMatrizPar[(((N/P)-2)*N) + j - 1] +
                    vBordeInferior[j-1] +
                    vBordeInferior[j] +
                    vBordeInferior[j+1]) * unNoveno;
            }
        }

        //Borde izquierdo sin esquinas y borde derecho sin esquinas
        for(i=1;i<N/P-1;i++){
            //Borde izquierdo
            mResP[N*i] = (
                subMatrizPar[N*i] +
                subMatrizPar[N*i+1] +
                subMatrizPar[N*(i-1)] +
                subMatrizPar[N*(i-1)+1] +
                subMatrizPar[N*(i+1)] +
                subMatrizPar[N*(i+1)+1]) * unSexto;

            //Borde derecho
            mResP[N*i+N-1] = (
                subMatrizPar[N*i+N-1] +
                subMatrizPar[N*i+N-2] +
                subMatrizPar[N*(i-1)+N-1] +
                subMatrizPar[N*(i-1)+N-2] +
                subMatrizPar[N*(i+1)+N-1] +
                subMatrizPar[N*(i+1)+N-2]) * unSexto;
        }


        //Promedio de los elementos medios de la subMatriz
       for(i=1;i<N/P-1;i++){
            for(j=1;j<N-1;j++){
                mResP[i*N+j]=(
                    subMatrizPar[(i-1)*N+j-1] +
                    subMatrizPar[(i)  *N+j-1] +
                    subMatrizPar[(i+1)*N+j-1] +
                    subMatrizPar[(i-1)  *N+j] +
                    subMatrizPar[(i)    *N+j] +
                    subMatrizPar[(i+1)  *N+j] +
                    subMatrizPar[(i-1)*N+j+1] +
                    subMatrizPar[(i)  *N+j+1] +
                    subMatrizPar[(i+1)*N+j+1]
                ) * unNoveno ;
            }
        }
        
        valorAComparar = mResP[0]; //Cada proceso toma el primer elemento como el valor a comparar

        //Para que todos los subvectores convergan al mismo valor, hace falta mandar dicho valor a todos los procesos.
        if(myrank == 0){
            timetickCom = dwalltime();
        }
        MPI_Bcast(&valorAComparar, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }

        subMatrizPar[0]=mResP[0];
        convergioLocal = true;
        

        for(i=1;(i<(N/P)*N);i++){
            subMatrizPar[i]=mResP[i];

            if(convergioLocal && !(fabs(valorAComparar-mResP[i])<0.01)){
                convergioLocal = false;
            }
        }
        //Reduce, usando la operacion logica AND, y envia el resultado a todos los procesos.
        if(myrank == 0){
            timetickCom = dwalltime();
        }
        MPI_Allreduce(&convergioLocal,&convergio,1,MPI_C_BOOL,MPI_LAND,MPI_COMM_WORLD);
        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }
    }

    if(myrank == 0){
        printf("Usando la estrategia paralela, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", valorAComparar, iteraciones, dwalltime() - timetick);
        printf("El overhead de comunicacion fue %f segundos\n", tComunicacion);
    }
    MPI_Finalize();

    return(0);

}