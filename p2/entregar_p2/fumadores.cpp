/*
Sistemas Concurridos y Distribuidos
ETSIIT, UGR
Practica 2: Monitores
Fumadores con monitor con espera urgente
*/


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

const int fumadores = 3;   // número de fumadores

mutex
   mtx ;                 // mutex de escritura en pantalla

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones fuera del monitor
//----------------------------------------------------------------------

void fumar ( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<200,2000>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador + 1 << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador + 1 << "  : termina de fumar, comienza espera de ingrediente." << endl;

}
//----------------------------------------------------------------------

int ProducirIngrediente (  )
{

  // calcular milisegundos aleatorios de duración de la acción de producir
  chrono::milliseconds producir( aleatorio<200,2000>() );

	int i;
  i = aleatorio<0,(fumadores - 1)>();
  // espera bloqueada un tiempo igual a 'producir milisegundos
  this_thread::sleep_for( producir );
	cout << "Producido ingrediente: " << i + 1 << endl;
	return i;
}
//----------------------------------------------------------------------

// *****************************************************************************
// clase para monitor Estanco

class EstancoSU : public HoareMonitor {
 private:
   int ingrediente_disp[fumadores];
   bool mostradorLibre;

   CondVar fumador[fumadores];
   CondVar mostrador;

 public:                    // constructor y métodos públicos
   EstancoSU();
   void ponerIngrediente( int num_fumador ); // Poner ingrediente en mostrador
   void obtenerIngrediente( int num_fumador ); // Obtener ingrediente del mostrador
   void esperarRecogidaIngrediente( ); // Esperar a que mostrador este libre
 };
// -----------------------------------------------------------------------------

EstancoSU::EstancoSU(  )
{
   mostradorLibre = true;
   for (int i = 0; i < fumadores; i++){
     fumador[i] = newCondVar();
     ingrediente_disp[fumadores] = 0;
   }
   mostrador = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el estanquero para poner un ingrediente

void EstancoSU::ponerIngrediente( int num_fumador )
{
   cout << "Puesto ingrediente " << num_fumador + 1 << " en mostrador." << endl;
   ingrediente_disp[num_fumador] = 1;
   mostradorLibre = false;

   // señalar al fumador que esta su ingrediente, por si está esperando
   fumador[num_fumador].signal();
}
// -----------------------------------------------------------------------------
// función llamada por el fumador para recoger un ingrediente

void EstancoSU::obtenerIngrediente( int num_fumador )
{
   if ( ingrediente_disp[num_fumador] == 0 ){
      fumador[num_fumador].wait();
   }

   cout << "Retirado ingrediente " << num_fumador + 1 << " del mostrador." << endl;
   ingrediente_disp[num_fumador] = 0;
   mostradorLibre = true;

   // señalar al estanquero que ya esta libre el mostrador
   mostrador.signal();
}
// -----------------------------------------------------------------------------
// función llamada por el fumador para esperar recogida

void EstancoSU::esperarRecogidaIngrediente( )
{
  if ( mostradorLibre == false ) {
     mostrador.wait();
  }

}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_fumador( MRef<EstancoSU> Estanco , int num_fumador )
{
  while(true){
    Estanco->obtenerIngrediente(num_fumador);
    fumar(num_fumador);
  }
}
// -----------------------------------------------------------------------------

void funcion_hebra_estanquero( MRef<EstancoSU> Estanco )
{
  int ingre;
  while(true){
  ingre = ProducirIngrediente();
  Estanco->ponerIngrediente (ingre);
  Estanco->esperarRecogidaIngrediente();
  }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "FUMADORES Monitor SU, con " << fumadores << " fumadores." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<EstancoSU> Estanco = Create<EstancoSU>();

   // declarar el array de variables de tipo 'thread'
   thread hebras[fumadores+1];
   // poner en marcha todas las hebras fumadoras
   for( int i = 0 ; i < fumadores ; i++ )
     hebras[i] = thread( funcion_hebra_fumador, Estanco, i ) ;
   // poner en marcha la hebra estanquero
     hebras[fumadores] = thread( funcion_hebra_estanquero, Estanco ) ;
   // esperar a que terminen todas las hebras
   for( int i = 0 ; i < (fumadores + 1) ; i++ )
     hebras[i].join() ;

}
