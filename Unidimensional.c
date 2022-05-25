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
		v[j] = ((float)rand())/RAND_MAX;
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

    float* swap;
    float* vSec;
    float* vSecConvergido;
    float* vPar;
    float* vParConvergido;

    int i, iteraciones = 0;
    bool convergio = false;
    
    //Se almacena el resultado de esta operación para no tener que repetirla cada iteración
    float unTercio = 1.0/3.0;
    register float valorAComparar;

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
        valorAComparar = vSecConvergido[0];
        //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
        for(i=1;i<N & convergio;i++){
            //printf("v[0]: %.2f\n", fabs(vSecConvergido[0] - vSecConvergido[i]));
            if(!(fabs(valorAComparar - vSecConvergido[i])<0.01)){
                convergio = false;
                swap = vSec;
                vSec = vSecConvergido;
                vSecConvergido = swap;
            }
        }
        //printf("v[0]: %.2f\n", vSecConvergido[0]);
    }
    //TODO: calcular tiempo
    printf("Usando la estrategia secuencial en una dimension, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", vSecConvergido[0], iteraciones, dwalltime() - timetick);

    //ESTRATEGIA DE RESOLUCION PARALELA:
    //Calcular promedio -> barrera -> chequear convergencia

    convergio = false;
    iteraciones = 0;
    timetick = dwalltime();
    while(!convergio){
        vParConvergido[0] = ((vPar[0] + vPar[1]) * 0.5);
        vParConvergido[N-1] = ((vPar[N - 1] + vPar[N - 2]) * 0.5);
        iteraciones++;

        //TODO: pensar que scheduler usar
        //Al momento de subir al cluster, recordar borrar num_threads() ya que se usa la variable del sistema.
        //Si private(i) ya está en el parallel, en el for es redundante
        #pragma omp parallel private(i) shared(vPar, vParConvergido)
        {
            #pragma omp for
            for(i=1;i<N-1;i++){
                vParConvergido[i] = ((vPar[i - 1] + vPar[i] + vPar[i+1]) * unTercio);
                //printf("vParConv[%i] = %f\n", i, vParConvergido[i]);
            }
            
            #pragma omp single
            {
                convergio = true;
                valorAComparar = vParConvergido[0];
            }
            
            //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
            //Se utiliza la directiva de reduccion para que al final de la seccion paralela se realize una operacion AND sobre la variable booleana de convergencia.
            #pragma omp for reduction(&& : convergio)
            //for(int i=1;i<N*N & convergio;i++) tira invalid controller predicate con pragma omp
            //tampoco puedo usar un break
            for(i=1;i<N;i++){
                //printf("v[0]: %.2f\n", fabs(vSecConvergido[0] - vSecConvergido[i]));
                if(!(fabs(vParConvergido[0] - vParConvergido[i])<0.01)){
                    convergio = false;
                    swap = vPar;
                    vPar = vParConvergido;
                    vParConvergido = swap;
                }
            }
        }
        //printf("v[0]: %.2f\n", vSecConvergido[0]);
    }
    printf("Usando la estrategia paralela en una dimension, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", vParConvergido[0], iteraciones, dwalltime() - timetick);


    return 0;
}
