#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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

//Inicializa un vector
float* inicializarVector(float* v,unsigned long N){
	for(int j=0;j<N;j++){
        //Genera un numero "aleatorio" entre 1 y 10
        //No se está usando srand() con una semilla, por tanto siempre retorna los mismos numeros, pero los vectores siguen siendo distintos.
		v[j]= ((float)rand())/RAND_MAX;
	}
	return v;
}

int main(int argc, char* argv[]){

    //Se comprueba que se hayan pasado la cantidad correcta de parámetros de entrada. Evita que tire un coredump en caso de que se corra mal el script. 
    if ((argc != 3) || (atoi(argv[1]) <= 0) || (atoi(argv[1]) <= 0)){
        printf("\nUsar: %s N T\n N: Dimension del vector\n T: Cantidad de threads\n", argv[0]);
        exit(1);
    }
    //srand(time(0));
    //Tamano del vector
    unsigned long N = atol(argv[1]);
    unsigned long numBytes = sizeof(float)*N;
    //Cantidad de Procesos. Dado que el enunciado especificaba que el programa debe correr para 4 y 8 procesos, no hace falta usar una variable mas grande.
    unsigned char P = atol(argv[2]);
    
    int myrank;
    double timetick, timetickCom, tComunicacion = 0; //timetickCom y tComunicacion se usan para medir el overhead de comunicacion. Si bien empeoran el tiempo de ejecucion, son necesarias para realizar un analisis de escalabilidad apropiado.

    float* swap;
    float* vSec;
    float* vSecConvergido;
    float* vPar;
    float* vParConvergido;

    int *reparto = (int*)malloc(sizeof(int)*P); //Vector con cantidad de elementos repartidos a cada proceso
    int *despl = (int*)malloc(sizeof(int)*P); //Vector que indica el desplazamiento desde la primera posicion del proceso anterior al siguiente

    //Se almacena el resultado de esta operacion para no tener que repetirla cada iteracion
    float unTercio = 1.0/3.0;

    bool convergio = false;
    int i, iteraciones = 0;
    float valorAComparar;

    MPI_Status status;
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    if(myrank == 0){
        printf("Se ejecuta el algoritmo de convergencia para N = %i P = %i\n",N,P);
        printf("----------------------------------------------------------\n");

        //Se reserva memoria para los vectores
        vSec=(float*)malloc(numBytes);
        vSecConvergido=(float*)malloc(numBytes);
        vPar=(float*)malloc(numBytes);
        vParConvergido=(float*)malloc(numBytes);
        
        //Se inicializan los vectores
        vSec=inicializarVector(vSec,N);
        //Para poder hacer una comparacion apropiada, ambas estrategias van a trabajar sobre el mismo set de datos
        for(i=0;i<N;i++){
            vPar[i] = vSec[i];
        }
        
        //Resolucion secuencial
        //Se evita encapsular secciones de codigo como funciones para optimizar el tiempo de procesamiento.
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
    }

    //Procesamiento paralelo con MPI

    convergio = false;
    float *subVectorPar;
    float *vResP;

    //Variables usadas por cada proceso que contienen el elemente anterior o siguiente de su primera o ultima posicion, respectivamente.
    //Usadas para calcular el promedio de sus valores bordes.
    float valorBordeSuperior, valorBordeInferior;
    
    subVectorPar = (float*)malloc(sizeof(float)*(N/P));
    vResP = (float*)malloc(sizeof(float)*N/P); 

    iteraciones = 0;
    bool convergioLocal = true; //variable de convergencia local que sera reducida
    //Barrera para asegurarse de que todos los procesos esperen a que 0 termine de ejecutar el algoritmo secuencial.
    MPI_Barrier(MPI_COMM_WORLD);

    int cantidadDeElementosPorSubVector = ((int)N/(int)P);
    printf("Soy el proceso nro %i\n",myrank);

    if(myrank == 0){
        timetickCom = dwalltime();
    }
    timetick = dwalltime();  //Para MPI tenemos en cuenta el overhead de comunicacion, que arranca ahora
    MPI_Scatter(vPar,N/P,MPI_FLOAT,subVectorPar,N/P,MPI_FLOAT,0,MPI_COMM_WORLD); //cada proceso recibe una parte del vector original
    if(myrank == 0){
        tComunicacion = tComunicacion + (dwalltime() - timetickCom);
    }


    while(!convergio){
        iteraciones++;

	    //Cada proceso le manda su ultimo valor al siguiente, en orden para asegurarse que no ocurran deadlocks
        if(myrank == 0){
            timetickCom = dwalltime();
        }

	    if(myrank == 0){
	        MPI_Send(&subVectorPar[(N/P)-1], 1, MPI_FLOAT, 1, 0, MPI_COMM_WORLD);
	    }
	    if(myrank > 0 && myrank < P-1){
	        MPI_Recv(&valorBordeInferior, 1, MPI_FLOAT, myrank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	        MPI_Send(&subVectorPar[(N/P)-1], 1, MPI_FLOAT, myrank+1, 0, MPI_COMM_WORLD);
	    }
	    if(myrank == P-1){
	        MPI_Recv(&valorBordeInferior, 1, MPI_FLOAT, P-2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    }

	    //Ahora que se tienen todos los valores del borde inferior, se repite la cadena de mensajes pero en sentido inverso
	    if(myrank == P-1){
	        MPI_Send(&subVectorPar[0], 1, MPI_FLOAT, P-2, 0, MPI_COMM_WORLD);
	    }
	    if(myrank > 0 && myrank < P-1){
	        MPI_Recv(&valorBordeSuperior, 1, MPI_FLOAT, myrank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	        MPI_Send(&subVectorPar[0], 1, MPI_FLOAT, myrank-1, 0, MPI_COMM_WORLD);
	    }
	    if(myrank == 0){
	        MPI_Recv(&valorBordeSuperior, 1, MPI_FLOAT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    }

        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }

        //Se calcula el promedio de todos los elementos del medio. Esto es identico para todos los procesos.
        for(i=1;i<N/P-1;i++){
            vResP[i] = (subVectorPar[i-1]+subVectorPar[i]+subVectorPar[i+1])*unTercio;
        }
        
        //Luego se calcula el promedio del primer elemento. El proceso 0 es un caso especial.
        if(myrank == 0){
            vResP[0] = (subVectorPar[0]+subVectorPar[1])*0.5; 
        }
        //El resto de los procesos necesita el valor borde inferior.
        else if(myrank > 0){
            vResP[0] = (valorBordeInferior+subVectorPar[0]+subVectorPar[1])*unTercio;
        }

        //Finalmente se calcula el promedio del ultimo elemento. El ultimo proceso es un caso especial
        if(myrank < P-1){
            vResP[(N/P)-1] = (subVectorPar[N/P-2]+subVectorPar[N/P-1]+valorBordeSuperior)*unTercio;
        }
        else if(myrank == P-1){
            vResP[(N/P)-1] = (subVectorPar[N/P-2]+subVectorPar[N/P-1])*0.5;
        }
        
        valorAComparar = vResP[0]; //Cada proceso toma el primer elemento como el valor a comparar
        //Para que todos los subvectores convergan al mismo valor, hace falta mandar dicho valor a todos los procesos.
        if(myrank == 0){
            timetickCom = dwalltime();
        }
        MPI_Bcast(&valorAComparar, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }

		subVectorPar[0]=vResP[0];
        convergioLocal = true;
        //Dado que para todos los procesos menos el 0 el primer valor de su subvector no necesariamente es igual al de comparacion, se deben chequear todos.
        for(i=0;i<cantidadDeElementosPorSubVector;i++){
            //printf("Soy el proceso %i, loop de conv nro %i\n",myrank,i);
            subVectorPar[i]=vResP[i];
            if(!(fabs(valorAComparar-vResP[i])<0.01) & convergioLocal){
                convergioLocal = false;
            }
        }
        //Reduce, unsando la operacion logica AND, y envia el resultado a todos los procesos.
        if(myrank == 0){
            timetickCom = dwalltime();
        }
        MPI_Allreduce(&convergioLocal,&convergio,1,MPI_C_BOOL,MPI_LAND,MPI_COMM_WORLD);
        if(myrank == 0){
            tComunicacion = tComunicacion + (dwalltime() - timetickCom);
        }
        
    }
    //MPI_Scatter(vPar,N/P,MPI_FLOAT,subVectorPar,N/P,MPI_FLOAT,0,MPI_COMM_WORLD);
    if(myrank == 0){
        timetickCom = dwalltime();
    }
    MPI_Gather(vResP,N/P,MPI_FLOAT,vPar,N/P,MPI_FLOAT,0,MPI_COMM_WORLD);
    if(myrank == 0){
        tComunicacion = tComunicacion + (dwalltime() - timetickCom);
    }

    if(myrank == 0){
        printf("Usando la estrategia paralela, convergio al valor: %.2f en %i iteraciones que tomaron %f segundos\n", valorAComparar, iteraciones, dwalltime() - timetick);
        printf("Tiempo total de comunicacion calculado: %f\n", tComunicacion);
    }
 
    MPI_Finalize();

    return(0);

}
