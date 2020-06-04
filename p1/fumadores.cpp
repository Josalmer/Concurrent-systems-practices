/*
Sistemas Concurridos y Distribuidos
ETSIIT, UGR              
Practica 1: Sincronización de hebras con semáforos
*/

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

// variables compartidas
const int fumadores = 3;   // número de FUMADORES

// Incialización de semáforos
Semaphore ingrediente_disp[fumadores] = {0, 0, 0}; // fumadores sin ingredientes al empezar
Semaphore mostrador = 1; // estado del mostrador, empieza en 1 porque hay espacio para un ingrediente

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
	int i;
   	while(true){
		i = aleatorio<0,2>();
		sem_wait(mostrador);
		cout << "Producido ingrediente: " << i << endl;
		sem_signal(ingrediente_disp[i]);
   	}
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<200,2000>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   	while(true){
		sem_wait(ingrediente_disp[num_fumador]);
		cout << "Retirado ingrediente: " << num_fumador << endl;
		sem_signal(mostrador);
		fumar(num_fumador);
   	}
}

//----------------------------------------------------------------------

int main()
{
  // declarar el array de variables de tipo 'thread'
  thread hebras[fumadores+1];
  // poner en marcha todas las hebras fumadoras
  for( int i = 0 ; i < fumadores ; i++ )
    hebras[i] = thread( funcion_hebra_fumador, i ) ;
  // poner en marcha la hebra mostrador
    hebras[3] = thread( funcion_hebra_estanquero ) ;
  // esperar a que terminen todas las hebras
  for( int i = 0 ; i < (fumadores + 1) ; i++ )
    hebras[i].join() ;
}
