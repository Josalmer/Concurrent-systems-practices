/*
Sistemas Concurridos y Distribuidos
ETSIIT, UGR
Seminario 2: Monitores
Productor-Consumidor con cola FIFO y monitor con espera urgente
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

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

const int nproductores = 8;   // número de nproductores
const int nconsumidores = 4;  // número de consumidores
int prodxHebra = num_items / nproductores;
int producidosporhebra[nproductores] = {0};

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos

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
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int numhebra)
{
   int ip = numhebra * prodxHebra;
   int contador = producidosporhebra[numhebra] + ip;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   producidosporhebra[numhebra] ++ ;
   return contador ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SU, nproductores prod. y nconsumidores cons.

class ProdConsFIFOSU : public HoareMonitor {
 private:
   static const int num_celdas_total = 10;
   int buffer[num_celdas_total];
   int primera_libre;
   int primera_ocupada;
   int celdas_ocupadas;

   CondVar ocupadas;
   CondVar libres;

 public:                    // constructor y métodos públicos
   ProdConsFIFOSU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsFIFOSU::ProdConsFIFOSU(  )
{
   primera_libre = 0;
   primera_ocupada = 0;
   celdas_ocupadas = 0;
   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsFIFOSU::leer(  )
{
   if ( celdas_ocupadas == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada = (primera_ocupada + 1) % num_celdas_total ;
   celdas_ocupadas--;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsFIFOSU::escribir( int valor )
{
   if ( celdas_ocupadas == num_celdas_total )
      libres.wait();

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre = (primera_libre + 1) % num_celdas_total ;
   celdas_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsFIFOSU> monitor , int indiceHebra )
{
   for( unsigned i = 0 ; i < prodxHebra ; i++ )
   {
      int valor = producir_dato(indiceHebra) ;
      monitor->escribir( valor );
//      cout << endl << "Hebra productora " << indiceHebra << " escribe dato " << valor << endl;
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsFIFOSU> monitor )
{
   for( unsigned i = 0 ; i < (num_items / nconsumidores) ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores Monitor SU, buffer FIFO con " << nproductores << " productores y " << nconsumidores << " consumidores." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsFIFOSU> monitor = Create<ProdConsFIFOSU>();

   // Hebras productoras
   thread hebrasprod[nproductores];
   for( int i = 0 ; i < nproductores ; i++ ) {
     hebrasprod[i] = thread( funcion_hebra_productora , monitor, i ) ;
   }

   // Hebras consumidoras
   thread hebrascons[nconsumidores];
   for( int i = 0 ; i < nconsumidores ; i++ ) {
     hebrascons[i] = thread( funcion_hebra_consumidora, monitor ) ;
   }

   // Esperamos a que acaben todas las hebras
   for( int i = 0 ; i < nproductores ; i++ ) {
     hebrasprod[i].join();
   }
   for( int i = 0 ; i < nconsumidores ; i++ ) {
     hebrascons[i].join();
   }

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
