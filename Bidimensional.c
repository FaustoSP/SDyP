#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include <stdbool.h>
#include <math.h>

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
    if ((argc != 2) || (atoi(argv[1]) <= 0) ){
        printf("\nUsar: %s n\n  n: Dimension del vector\n", argv[0]);
        exit(1);
    }

    double timetick;

    float* swap;
    float* mSec;
    float* mSecConvergido;
    float* vPar;
    //Para verificar la convergencia se toma el primer elemento ( V[0] ) y se compara con el resto de los elementos del vector. Si la diferencia en valor
    //absoluto del valor del primer elemento con todos los elementos restantes es menor a un valor de precisión el algoritmo converge, en caso contrario el
    //algoritmo no converge.
    //TODO: preguntar de donde saco el valor de precisión
    int i, j, iteraciones = 0;
    bool convergio = false;
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N*N;

    //Se inicializan las matrices
    mSec=(float*)malloc(numBytes);
    mSec=inicializarMatriz(mSec,N);

    mSecConvergido=(float*)malloc(numBytes);

    bool firstTime = true;
    timetick = dwalltime();
    float unCuarto = 1./4.0;
    float unSexto = 1./6.0;   
    float unNoveno = 1./9.0;
    register float valorAComparar;
    while(!convergio){
        iteraciones++;

        //La posicion de un valor de la matriz se calcula como i*N+j

        //Primero se calcula el valor de las esquinas. Notese que le indice maximo para cada variable es N-1, no N.
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

        //para debugear, borrar despues
        /*if(firstTime){
            for(int i=0;i<N;i++){
                for(int j=0;j<N;j++){
                    printf("mSecConv[%i,%i]: %.2f\n", i, j, mSecConvergido[i*N+j]);
                }
            }
            //firstTime = false;
        }
        */

        //Se evalúa si todos los valores del vector convergieron.
        convergio = true;
        valorAComparar = mSecConvergido[0];
        for(int i=1;i<N*N & convergio;i++){
            //printf("m[%i]: %.2f\n",i*N+j, mSecConvergido[i*N+j]);
            //mSec[i*N+j] = mSecConvergido[i*N+j];
            //Si algún valor no estuviera por debajo del límite de precisión, se copia el valor del vector actual al original
            //printf("m[%lu]: %.2f\n",i*N+j, fabs(mSecConvergido[0] - mSecConvergido[i*N+j]));
            if(!(fabs(valorAComparar - mSecConvergido[i])<0.01)){
                convergio = false;
                swap = mSec;
                mSec = mSecConvergido;
                mSecConvergido = swap;
            }
        }
    }
    printf("Convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", mSecConvergido[0], iteraciones, dwalltime() - timetick);


    //SOLO SE PARALELIZA EL FOR EXTERIOR


    return 0;
}