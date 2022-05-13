#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include <stdbool.h>

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
	        m[i*N+j]=(rand()%10) + 1;
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

    float* mSecOri;
    float* mSecConvergido;
    float* vPar;
    //Para verificar la convergencia se toma el primer elemento ( V[0] ) y se compara con el resto de los elementos del vector. Si la diferencia en valor
    //absoluto del valor del primer elemento con todos los elementos restantes es menor a un valor de precisión el algoritmo converge, en caso contrario el
    //algoritmo no converge.
    //TODO: preguntar de donde saco el valor de precisión
    int valorDePrecisionDeConvergencia, i, iteraciones = 0;
    bool convergio = false;
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N*N;

    //Se inicializan las matrices
    mSecOri=(float*)malloc(numBytes);
	mSecOri=inicializarMatriz(mSecOri,N);

    mSecConvergido=(float*)malloc(numBytes);

    while(!convergio){
        iteraciones++;
        //Se calcula el promedio entre cada valor y sus vecinos
        for(i=0;i<N;i++){
            if(i == 0){
                mSecConvergido[i] = ((mSecOri[0] + mSecOri[1]) / 2);
            } else {
                if(i == N-1){
                mSecConvergido[i] = ((mSecOri[N - 1] + mSecOri[N - 2]) / 2);
                } else {
                    mSecConvergido[i] = ((mSecOri[i - 1] + mSecOri[i] + mSecOri[i+1]) / 3);
                }
            }
            //printf("%.2f\n", mSecConvergido[i]); TODO: borrar
	    }

        //Se evalúa todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
        convergio = true;
        for(i=1;i<N;i++){
            //TODO: el valor de precision está hardcodeado, cambiar
            //Si algún valor no estuviera por debajo del límite de precisión, se copia el valor del vector actual al original
            //printf("v[0]: %.2f\n", fabs(mSecConvergido[0] - mSecConvergido[i]));
            if(!(fabs(mSecConvergido[0] - mSecConvergido[i])<0.1)){
                convergio = false;
                //TODO: Hay alguna manera mejor que usar memcpy?
                memcpy(mSecOri, mSecConvergido, numBytes);
            }
        }
        //printf("v[0]: %.2f\n", mSecConvergido[0]);
    }
    printf("Convergio al valor: %.2f en %i iteraciones\n", mSecConvergido[0], iteraciones);

    return 0;
}