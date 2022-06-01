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
		v[j] = ((float)rand())/RAND_MAX;
	}
	return v;
}

int main(int argc, char* argv[]){

    //Se comprueba que se hayan pasado la cantidad correcta de parámetros de entrada. Evita que tire un coredump en caso de que se corra mal el script. 
    if ((argc != 2) || (atoi(argv[1]) <= 0)){
        printf("\nUsar: %s N\n N: Dimension del vector\n", argv[0]);
        exit(1);
    }
    //srand(time(0)); se deja comentado para que la semilla quede fija
    //Tamaño del vector
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N;
    
    double timetick;

    float* swap;
    float* vSec;
    float* vSecConvergido;
    float* vPar;
    float* vParConvergido;

    int i;
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

        //Se calcula el promedio entre cada valor y sus vecinos
        vSecConvergido[0] = ((vSec[0] + vSec[1]) * 0.5);
        vSecConvergido[N-1] = ((vSec[N - 1] + vSec[N - 2]) * 0.5);
        for(i=1;i<N-1;i++){
            vSecConvergido[i] = ((vSec[i - 1] + vSec[i] + vSec[i+1]) * unTercio);
	    }

        convergio = true;
        valorAComparar = vSecConvergido[0];
        //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
        for(i=1;i<N;i++){
            if(!(fabs(valorAComparar - vSecConvergido[i])<0.01) & convergio){
                convergio = false;
                swap = vSec;
                vSec = vSecConvergido;
                vSecConvergido = swap;
            }
        }
    }
    printf("Usando la estrategia secuencial en una dimension, convergio al valor: %.2f en %f segundos\n", vSecConvergido[0], dwalltime() - timetick);

    convergio = false;
    timetick = dwalltime();
    while(!convergio){
        
        //Scheduler estatico funciona mejor para este problema dado que las cargas de trabajo son identicas
        //En teoria, eliminar shared(...) tendria que mejorar el tiempo, pero de acuerdo en nuestros experimentos si hay un cambio es lo suficientemente pequeño como para que no se note
        #pragma omp parallel private(i) shared(vPar, vParConvergido)
        {
            #pragma omp for
            for(i=1;i<N-1;i++){
                vParConvergido[i] = ((vPar[i - 1] + vPar[i] + vPar[i+1]) * unTercio);
            }
            
            //Se descubrio que envolver todo este codigo en una sola directiva single reduce el tiempo de ejecucion paralelo. Esto probablemente se deba a la barrera implicita al final de single
            //Dado que la verificación de convergencia utiliza valorAComparar, no se puede agregar un nowait a esta seccion sin arriesgar a que ocasione un error
            #pragma omp single
            {
                vParConvergido[0] = ((vPar[0] + vPar[1]) * 0.5);
                vParConvergido[N-1] = ((vPar[N - 1] + vPar[N - 2]) * 0.5);
                convergio = true;
                valorAComparar = vParConvergido[0];
            }
            
            //Se evalúa si todos los valores del vector convergieron. Se saltea la primera posición dado que esa se toma como el valor de convergencia.
            //Se utiliza la directiva de reduccion para que al final de la seccion paralela se realice una operacion AND sobre la variable booleana de convergencia.
            #pragma omp for reduction(&& : convergio)
            for(i=1;i<N;i++){
                if(!(fabs(vParConvergido[0] - vParConvergido[i])<0.01) & convergio){
                    convergio = false;
                    swap = vPar;
                    vPar = vParConvergido;
                    vParConvergido = swap;
                }
            }
        }
    }
    printf("Usando la estrategia paralela en una dimension, convergio al valor: %.2f en  %f segundos\n", vParConvergido[0], dwalltime() - timetick);


    return 0;
}
