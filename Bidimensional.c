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

//Inicializa una matriz
float* inicializarMatriz(float* m,unsigned long N){
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            m[i*N+j]=((float)rand())/(float)RAND_MAX;
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
    float* mPar;
    float* mParConvergido;

    int i, j;
    bool convergio = false;
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N*N;

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
    float unCuarto = 1./4.0;
    float unSexto = 1./6.0;
    float unNoveno = 1./9.0;
    register float valorAComparar;

    //Primero se resuelve el problema con la estrategia secuencial
    while(!convergio){

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
        for(int i=1;i<N*N;i++){
            //Si algún valor no estuviera por debajo del límite de precisión, se copia el valor del vector actual al original
            if(!(fabs(valorAComparar - mSecConvergido[i])<0.01) & convergio){
                convergio = false;
                swap = mSec;
                mSec = mSecConvergido;
                mSecConvergido = swap;
            }
        }
    }
    printf("Usando la estrategia secuencial en dos dimensiones, convergio al valor: %.2f en %f segundos\n", mSecConvergido[0], dwalltime() - timetick);

    //Ahora se utiliza la estrategia paralela.
    convergio = false;

    timetick = dwalltime();
    while(!convergio){

        //La posicion de un valor de la matriz se calcula como i*N+j

        //Primero se calcula el valor de las esquinas. Notese que le indice maximo para cada variable es N-1, no N.
        //Idem a la version unidimensional, scheduler estatico funciona mejor para este problema, y no se observo que shared(...) tenga un impacto medible en el tiempo de ejecucion
        #pragma omp parallel private(i,j) shared(mPar, mParConvergido)
        {
            //Esquina superior izquierda (0,0).
            //Quiza seria mas eficiente unificar todas estas secciones en un solo omp single, pero se testeo y no se notaron cambios relevantes en el tiempo de ejecucion.
            //Esto probablemente se deba a que los distintos procesos se solapan en la ejecucion de su single, lo cual genera un ocultamiento de latencia que contraresta el efecto negativo de las barreras implicitas
            //Tambien vale aclarar que si se concentraran todos las secciones singles en una sola, un solo proceso tendría que ejecutar toda esa seccion, en ves de repartirse entre varios.
            #pragma omp single
            {
                mParConvergido[0] = ((
                    mPar[0] +
                    mPar[1] +
                    mPar[N] +
                    mPar[N+1]) * unCuarto);
            }

            //Esquina superior derecha (0,N-1).
            #pragma omp single
            {
                mParConvergido[N-1] = ((
                    mPar[N-1] + 
                    mPar[N-2] + 
                    mPar[(2*N)-1] + 
                    mPar[(2*N)-2]) * unCuarto);
            }

            //Esquina inferior izquierda (N-1,0).
            #pragma omp single
            {
                mParConvergido[(N-1)*N] = ((
                    mPar[(N-1)*N] + 
                    mPar[(N-1)*N+1] + 
                    mPar[(N-2)*N] + 
                    mPar[(N-2)*N+1]) * unCuarto);
            }

            //Esquina inferior derecha (N-1,N-1)
            #pragma omp single
            {
                mParConvergido[N*N -1] = ((
                    mPar[N*N -1] + 
                    mPar[N*N -2] + 
                    mPar[(N-1)*N -1] + 
                    mPar[(N-1)*N -2]) * unCuarto);
            }

            //Luego se calcula el valor de los bordes, menos las esquinas.
            #pragma omp for
            for(j=1;j<N-1;j++){
                mParConvergido[j]=(
                    mPar[j] +
                    mPar[j-1] +
                    mPar[j+1] +
                    mPar[j+N] +
                    mPar[j+N+1] +
                    mPar[j+N-1]) * unSexto;

                mParConvergido[N*(N-1)+j]=(
                    mPar[N*(N-1)+j] +
                    mPar[N*(N-1)+j-1] +
                    mPar[N*(N-1)+j+1] +
                    mPar[N*(N-2)+j] +
                    mPar[N*(N-2)+j+1] +
                    mPar[N*(N-2)+j-1]) * unSexto;    
            }

            #pragma omp for
            for(i=1;i<N-1;i++){
                mParConvergido[i*N] = (
                    mPar[i*N] +
                    mPar[i*N+1] +
                    mPar[(i-1)*N] +
                    mPar[(i-1)*N+1] +
                    mPar[(i+1)*N] +
                    mPar[(i+1)*N+1]) * unSexto;

                mParConvergido[i*N+(N-1)] = (
                    mPar[i*N+(N-1)] +
                    mPar[i*N+(N-1)-1] +
                    mPar[(i-1)*N+(N-1)] +
                    mPar[(i-1)*N+(N-1)-1] +
                    mPar[(i+1)*N+(N-1)] +
                    mPar[(i+1)*N+(N-1)-1]) * unSexto;
            }

            //Luego se calcula el promedio de la matriz interna, para 1<i<N-1, 1<j<N-1
            #pragma omp for
            for(i=1;i<N-1;i++){
                for(j=1;j<N-1;j++){
                    mParConvergido[i*N+j]=(
                        mPar[(i-1)*N+j-1] +
                        mPar[(i)  *N+j-1] +
                        mPar[(i+1)*N+j-1] +
                        mPar[(i-1)  *N+j] +
                        mPar[(i)    *N+j] +
                        mPar[(i+1)  *N+j] +
                        mPar[(i-1)*N+j+1] +
                        mPar[(i)  *N+j+1] +
                        mPar[(i+1)*N+j+1]
                    ) * unNoveno ;
                }
            }

            //Se evalúa si todos los valores del vector convergieron.
            #pragma omp single
            {
                convergio = true;
                valorAComparar = mParConvergido[0];
            }

            //Igual quen unidimensional, se utiliza un reduce
            #pragma omp for reduction(&& : convergio)
            for(int i=1;i<N*N; i++){
                if(!(fabs(valorAComparar - mParConvergido[i])<0.01) & convergio){
                    convergio = false;
                    swap = mPar;
                    mPar = mParConvergido;
                    mParConvergido = swap;
                }
            }
        }
    }
    printf("Usando la estrategia paralela en dos dimensiones, convergio al valor: %.2f en %f segundos\n", mParConvergido[0], dwalltime() - timetick);


    return 0;
}