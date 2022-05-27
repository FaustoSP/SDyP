#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>

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
    //Cantidad de Procesos. Dado que el enunciado especificaba que el programa debe correr para 4 y 8 procesos, no hace falta usar una variable más grande.
    unsigned char P = atol(argv[2]);
    
    int myrank;
    double timetick;

    float* vSec;
    float* vSecConvergido;
    float* vPar;
    float* vParConvergido;

    int *reparto = (int*)malloc(sizeof(int)*P); //Vector con cantidad de elementos repartidos a cada proceso
    int *despl = (int*)malloc(sizeof(int)*P); //Vector que indica el desplazamiento desde la primera posicion del proceso anterior al siguiente

    //Se almacena el resultado de esta operación para no tener que repetirla cada iteración
    float unTercio = 1.0/3.0;

    bool convergio = false;

    MPI_Status status;
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    if(myrank == 0){
        int i, iteraciones = 0;

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
        
        printf("Usando la estrategia secuencial, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", vSecConvergido[0], iteraciones, dwalltime() - timetick);
    }

    //PROCESAMIENTO PARALELO CON MPI

    convergio = false;
    float *vProm;
    float *vResP;

    float valorAComparar; //contiene el valor de convergencia
    
    //Seccion del vector que trabajara cada proceso
    if(myrank == 0 || myrank == P-1)
        vProm = (float*)malloc(sizeof(float)*(N/P+1)); //Variable para los fragmentos extremos del vector
    else
        vProm = (float*)malloc(sizeof(float)*(N/P+2)); //Variable para los fragmentos internos del vector

    vResP = (float*)malloc(sizeof(float)*N/P); //Vector de promedios de la seccion de cada proceso

    //Se fija la cantidad de elementos a repartir por proceso
    if(P > 1){
        int i;
        int j;

        reparto[0] = N/P+1;
        reparto[P-1] = N/P+1;

        despl[0] = 0;
        despl[P-1] = N/P;

        for(j = 1;j < P-2;j++){
            reparto[j] = N/P+2;
            despl[i] = N/P+1;
        }

    }

    if(myrank == 0){
        vSec = inicializarVector(vSec,N); //Se vuelve a inicializar el vector
    }

    timetick = dwalltime();    

    while(!convergio){
        int i;

        MPI_Scatterv(vSec,reparto,despl,MPI_FLOAT,vProm,N,MPI_FLOAT,0,MPI_COMM_WORLD);

        //MPI_Scatter(vSec,N,MPI_FLOAT,vProm,N,MPI_FLOAT,0,MPI_COMM_WORLD); //Sustituir con Scatterv

        for(i=1;i<N/P-1;i++)
                vResP[i] = (vProm[i-1]+vProm[i]+vProm[i+1])*unTercio;

        if(myrank == 0){

            //Si se trata del proceso 0, por lo que tiene la esquina izquierda
            vResP[0] = (vProm[0]+vProm[1])*0.5; 
        }
        else if(myrank > 0){

            //Si se trata de un proceso sin la esquina izquierda
            vResP[0] = (vProm[0]+vProm[1]+vProm[2])*unTercio;
        }

        if(myrank < P){

            //Si se trata de un proceso sin esquina derecha
            vResP[N/P] = (vProm[N/P-1]+vProm[N/P]+vProm[N/P+1])*unTercio;
        }
        else if(myrank == P){

            //Si se trata del proceso P, que tiene la esquina derecha
            vResP[N/P] = (vProm[N/P-1]+vProm[N/P])*0.5;
        }
        
        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Gather(vResP,N/P,MPI_FLOAT,vSec,N,MPI_FLOAT,0,MPI_COMM_WORLD);

        valorAComparar = vSec[0]; //Se obtiene el promedio de la posición 0 para comparar

        MPI_Bcast(&valorAComparar,1,MPI_UNSIGNED,0,MPI_COMM_WORLD); //Se reparte entre los procesos

        convergio = true;
            
        for(i=0;(i<N/P)||!convergio;i++){
            convergio = (fabs(valorAComparar-vResP[i])<0.01);
        }

        MPI_Bcast(&convergio,1,MPI_UNSIGNED,0,MPI_COMM_WORLD); //Actualiza variable "convergio" en todos los procesos

        /*
        if(myrank == 0){
            convergio = true;
            
            for(i=0;(i<N/P)||!convergio;i++){
                convergio = (fabs(vSec[0]-vSec[i])<0.01);
            }

            MPI_Bcast(&convergio,1,MPI_UNSIGNED,0,MPI_COMM_WORLD); //Actualiza variable "convergio" en todos los procesos
        }
        */
        
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if(myrank == 0){
        printf("Converge a %.2f en %.4f segundos",valorAComparar,dwalltime()-timetick);
    }

}
