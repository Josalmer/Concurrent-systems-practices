/*
Sistemas Concurridos y Distribuidos
ETSIIT, UGR
Practica 2: Monitores
Barbero Durmiente con monitor con espera urgente
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

const int numeroDeclientes = 6;   // número de fumadores
const int tam_sala_espera = numeroDeclientes;; // tamaño de la sala de espera

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

void EsperarFueraBarberia ( int num_cliente )
{

   // calcular milisegundos aleatorios de duración de la acción de esperar)
   chrono::milliseconds duracion_espera( aleatorio<900,9000>() );

   // informa de que comienza a esperar

    cout << "Cliente " << num_cliente + 1 << "  :"
          << " empieza a esperar fuera (" << duracion_espera.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_espera' milisegundos
   this_thread::sleep_for( duracion_espera );

   // informa de que ha terminado de esperar

    cout << "Cliente " << num_cliente + 1 << "  : termina de esperar fuera de la barbería, entra a la barbería a pelarse." << endl;

}
//----------------------------------------------------------------------

void CortarPeloACliente ()
{

   // calcular milisegundos aleatorios de duración de la acción de pelar)
   chrono::milliseconds duracion_pelar( aleatorio<200,2000>() );

   // informa de que comienza a pelar

    cout << "Duracion del pelado (" << duracion_pelar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_pelar' milisegundos
   this_thread::sleep_for( duracion_pelar );

}
//----------------------------------------------------------------------

// *****************************************************************************
// clase para monitor Barberia

class BarberiaSU : public HoareMonitor {
 private:
   bool barbero_ocupado;
   int clientes_esperando;

   CondVar barbero_espera;
   CondVar cliente_Sala_espera;
   CondVar cliente_pelandose;

 public:                    // constructor y métodos públicos
   BarberiaSU();
   void cortarPelo( int num_cliente ); //
   void siguienteCliente(); //
   void finCliente( ); //
 };
// -----------------------------------------------------------------------------

BarberiaSU::BarberiaSU(  )
{
   barbero_ocupado = false;
   clientes_esperando = 0;
   barbero_espera = newCondVar();
   cliente_Sala_espera = newCondVar();
   cliente_pelandose = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el cliente para pelarse

void BarberiaSU::cortarPelo( int num_cliente )
{
   if (barbero_ocupado){
     clientes_esperando++;
     cout << "Clientes esperando en sala de espera: " << clientes_esperando << " clientes." << endl;
     cliente_Sala_espera.wait();
   }

   barbero_ocupado = true;

   // señalar al barbero que esta listo para pelar, por si está esperando
   barbero_espera.signal();

   // espera hasta que acaben de pelarlo
   cliente_pelandose.wait();
}
// -----------------------------------------------------------------------------
// función llamada por el barbero para hacer pasar a un cliente

void BarberiaSU::siguienteCliente( )
{
   if (!(clientes_esperando > 0)){
      barbero_espera.wait();
   }

   // señalar al cliente para que se pele
   if (!(cliente_Sala_espera.empty())){
     clientes_esperando = clientes_esperando - 1;
     cliente_Sala_espera.signal();
   }
   cout << "Barbero empieza a pelar a cliente." << endl;
}
// -----------------------------------------------------------------------------
// función llamada por el fumador para esperar recogida

void BarberiaSU::finCliente( )
{
  cout << "Barbero ha terminado de pelar a cliente." << endl;
  barbero_ocupado = false;
  cliente_pelandose.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_cliente( MRef<BarberiaSU> Barberia , int num_cliente )
{
  while(true){
    Barberia->cortarPelo(num_cliente);
    EsperarFueraBarberia(num_cliente);
  }
}
// -----------------------------------------------------------------------------

void funcion_hebra_barbero( MRef<BarberiaSU> Barberia )
{
  while(true){
  Barberia->siguienteCliente();
  CortarPeloACliente();
  Barberia->finCliente();
  }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "BARBERO DURMIENTE Monitor SU, con " << numeroDeclientes << " clientes." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<BarberiaSU> Barberia = Create<BarberiaSU>();

   // declarar el array de variables de tipo 'thread'
   thread hebras[numeroDeclientes+1];
   // poner en marcha todas las hebras clientes
   for( int i = 0 ; i < (numeroDeclientes - 1) ; i++ )
     hebras[i] = thread( funcion_hebra_cliente, Barberia, i ) ;
   // poner en marcha la hebra barbero
     hebras[numeroDeclientes] = thread( funcion_hebra_barbero, Barberia ) ;
   // esperar a que terminen todas las hebras
   for( int i = 0 ; i < (numeroDeclientes + 1) ; i++ )
     hebras[i].join() ;
}
