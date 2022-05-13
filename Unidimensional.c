#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include <stdbool.h>

//Toma demasiadas iteraciones, hay algo que no está andando mal

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

//Inicializa un vector
float* inicializarVector(float* v,unsigned long N){
	for(int j=0;j<N;j++){
        //Genera un numero "aleatorio" entre 1 y 10
        //No se está usando srand() con una semilla, por tanto siempre retorna los mismos números, pero los vectores siguen siendo distintos.
		v[j]= ((float)rand())/RAND_MAX;
        //printf("%.2f\n", v[j]);
	}
	//printf("------------------------------\n");
	return v;
}

int main(int argc, char* argv[]){

    //Se comprueba que se hayan pasado la cantidad correcta de parámetros de entrada. Evita que tire un coredump en caso de que se corra mal el script. 
    if ((argc != 2) || (atoi(argv[1]) <= 0) ){
        printf("\nUsar: %s n\n  n: Dimension del vector\n", argv[0]);
        exit(1);
    }

    //srand(time(0));

    float* vSecOri;
    float* vSecConvergido;
    float* vPar;
    //Para verificar la convergencia se toma el primer elemento ( V[0] ) y se compara con el resto de los elementos del vector. Si la diferencia en valor
    //absoluto del valor del primer elemento con todos los elementos restantes es menor a un valor de precisión el algoritmo converge, en caso contrario el
    //algoritmo no converge.
    //TODO: preguntar de donde saco el valor de precisión
    int valorDePrecisionDeConvergencia, i, iteraciones = 0;
    bool convergio = false;
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N;
    float unTercio = 1.0/3.0;

    //Se inicializan los vectores.
    vSecOri=(float*)malloc(numBytes);
	vSecOri=inicializarVector(vSecOri,N);

    vSecConvergido=(float*)malloc(numBytes);
    while(!convergio){
        iteraciones++;

        //Se calcula el promedio entre cada valor y sus vecinos
        vSecConvergido[0] = ((vSecOri[0] + vSecOri[1]) * 0.5);
        vSecConvergido[N-1] = ((vSecOri[N - 1] + vSecOri[N - 2]) * 0.5);
        for(i=1;i<N-1;i++){
            vSecConvergido[i] = ((vSecOri[i - 1] + vSecOri[i] + vSecOri[i+1]) * unTercio);
            //printf("%.2f\n", vSecConvergido[i]); TODO: borrar
	    }

        convergio = true;
        vSecOri[0] = vSecConvergido[0];
        //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
        for(i=1;i<N;i++){
            vSecOri[i] = vSecConvergido[i];
            //printf("v[0]: %.2f\n", fabs(vSecConvergido[0] - vSecConvergido[i]));
            if(!(fabs(vSecConvergido[0] - vSecConvergido[i])<0.01)){
                convergio = false;
            }
        }
        //printf("v[0]: %.2f\n", vSecConvergido[0]);
    }
    printf("Convergio al valor: %.2f en %i iteraciones\n", vSecConvergido[0], iteraciones);

    return 0;
}
