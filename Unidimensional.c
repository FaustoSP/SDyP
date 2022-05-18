#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>

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
    if ((argc != 3) || (atoi(argv[1]) <= 0) || (atoi(argv[1]) <= 0)){
        printf("\nUsar: %s N T\n N: Dimension del vector\n T: Cantidad de threads\n", argv[0]);
        exit(1);
    }
    //srand(time(0));
    //Tamaño del vector
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N;
    //Cantidad de threads. Dado que el enunciado especificaba que el programa debe correr para 4 y 8 hilos, no hace falta usar una variable más grande.
    unsigned char numThreads = atol(argv[2]);
    
    double timetick;

    float* vSec;
    float* vSecConvergido;
    float* vPar;
    float* vParConvergido;

    int i, iteraciones = 0;
    bool convergio = false;
    
    //Se almacena el resultado de esta operación para no tener que repetirla cada iteración
    float unTercio = 1.0/3.0;

    //Se reserva memoria para los vectores
    vSec=(float*)malloc(numBytes);
    vSecConvergido=(float*)malloc(numBytes);
    vPar=(float*)malloc(numBytes);
    vParConvergido=(float*)malloc(numBytes);
    
    //Se inicializan los vectores
    vSec=inicializarVector(vSec,N);
    //Para poder hacer una comparación apropiada, ambas estrategias van a trabajar sobre el mismo set de datos
    for(i=0;i<N;i++){
        vPar[i] = vSec[i];
    }
    
    //Resolución secuencial
    //Se evita encapsular secciones de código como funciones para optimizar el tiempo de procesamiento.
    timetick = dwalltime();
    while(!convergio){
        iteraciones++;

        //Se calcula el promedio entre cada valor y sus vecinos
        vSecConvergido[0] = ((vSec[0] + vSec[1]) * 0.5);
        vSecConvergido[N-1] = ((vSec[N - 1] + vSec[N - 2]) * 0.5);
        for(i=1;i<N-1;i++){
            vSecConvergido[i] = ((vSec[i - 1] + vSec[i] + vSec[i+1]) * unTercio);
            //printf("%.2f\n", vSecConvergido[i]); TODO: borrar
	    }

        convergio = true;
        vSec[0] = vSecConvergido[0];
        //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
        for(i=1;i<N;i++){
            vSec[i] = vSecConvergido[i];
            //printf("v[0]: %.2f\n", fabs(vSecConvergido[0] - vSecConvergido[i]));
            if(!(fabs(vSecConvergido[0] - vSecConvergido[i])<0.01)){
                convergio = false;
            }
        }
        //printf("v[0]: %.2f\n", vSecConvergido[0]);
    }
    //TODO: calcular tiempo
    printf("Usando la estrategia secuencial, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", vSecConvergido[0], iteraciones, dwalltime() - timetick);

    //ESTRATEGIA DE RESOLUCION PARALELA:
    //Calcular promedio -> barrera -> chequear convergencia

    convergio = false;
    //TODO: pensar que scheduler usar
    timetick = dwalltime();
    //Los hilos solo se deben generar una vez, usar solamente un parallel.
    while(!convergio){
        vParConvergido[0] = ((vPar[0] + vPar[1]) * 0.5);
        vParConvergido[N-1] = ((vPar[N - 1] + vPar[N - 2]) * 0.5);

        //TODO: cada hilo va a comparar su parte con v[0], entonces esta es una variable que comparten todos.
        #pragma omp parallel private(i) shared(vPar, vParConvergido) num_threads(numThreads)
        {
            #pragma omp for private(i)
            for(i=1;i<N-1;i++){
                vParConvergido[i] = ((vPar[i - 1] + vPar[i] + vPar[i+1]) * unTercio);
                //printf("vParConv[%i] = %f\n", i, vParConvergido[i]);
            }

            //Hace falta esta barrera? single tiene una implícita al final, pero no antes. Puede ser que vParConvergido[0] no esté calculado
            #pragma omp barrier
            #pragma omp single
            {
                convergio = true;
                vPar[0] = vParConvergido[0];
            }
            
            //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
            #pragma omp for private(i)
            for(i=1;i<N;i++){
                vPar[i] = vParConvergido[i];
                //printf("v[0]: %.2f\n", fabs(vSecConvergido[0] - vSecConvergido[i]));
                if(!(fabs(vParConvergido[0] - vParConvergido[i])<0.01)){
                    convergio = false;
                }
            }
        }
        //printf("v[0]: %.2f\n", vSecConvergido[0]);
    }
    printf("Usando la estrategia paralela, convergio al valor: %.2f en X iteraciones que tomaron %f segundos\n", vParConvergido[0], dwalltime() - timetick);


    return 0;
}
