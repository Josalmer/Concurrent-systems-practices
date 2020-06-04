/*
Sistemas Concurridos y Distribuidos
ETSIIT, UGR
Práctica 3. Implementación de algoritmos distribuidos con MPI
Multiples productores y consumidores

Para compilar
mpicxx -std=c++11 -o prodcons2-mu prodcons2-mu.cpp
Para ejecutar
mpirun -np 10 ./prodcons2-mu
*/

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int np = 4;   // número de nproductores
const int nc = 5;  // número de consumidores

const int
   num_procesos_esperado = (np+nc+1),
   num_items             = 40,
   tam_vector            = 10;        // Tamaño del buffer

// Identificadores de procesos
const int id_buffer = np;

int prodxProd = 10; // a producir por cada productor
int contProd[np] = {0}; // contador de producidos

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
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int numProductor)
{
    int ik = numProductor * prodxProd;
    int contador = contProd[numProductor] + ik;
    sleep_for( milliseconds( aleatorio<10,100>()) );
    contProd[numProductor]++;
    cout << "Productor ha producido valor " << contador << endl << flush;
    return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int numProductor)
{
   for ( unsigned int i= 0 ; i < prodxProd; i++ )
   {
      // producir valor
      int valor_prod = producir(numProductor);
      // enviar valor
      cout << "Productor " << numProductor << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int numConsumidor)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < (num_items / nc); i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, 1, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, MPI_ANY_TAG, MPI_COMM_WORLD,&estado );
      cout << "Consumidor ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
    const int etiq_productor = 0;
    const int etiq_consumidor = 1;
    int         buffer[tam_vector],      // buffer con celdas ocupadas y vacías
                valor,                   // valor recibido o enviado
                primera_libre       = 0, // índice de primera celda libre
                primera_ocupada     = 0, // índice de primera celda ocupada
                num_celdas_ocupadas = 0, // número de celdas ocupadas
                etiqueta_valida;

   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ((num_celdas_ocupadas < tam_vector) && (num_celdas_ocupadas > 0)){ // si se puede leer o escribir
        etiqueta_valida = MPI_ANY_TAG;
      } else if ( num_celdas_ocupadas < tam_vector ){
         etiqueta_valida = etiq_productor ;
      }       // si buffer con espacio para escribir
      else if ( num_celdas_ocupadas > 0 ) {
         etiqueta_valida = etiq_consumidor ;
      } // si buffer con elemento que leer

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_valida, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      int emisor = estado.MPI_SOURCE; // leer emisor del mensaje en metadatos
      if (emisor == 0 || emisor == 1 || emisor == 2 || emisor == 3){
        buffer[primera_libre] = valor ;
        primera_libre = (primera_libre+1) % tam_vector ;
        num_celdas_ocupadas++ ;
        cout << "Buffer ha recibido valor " << valor << endl ;
      } else if (emisor == 5 || emisor == 6 || emisor == 7 || emisor == 8 || emisor == 9){
        valor = buffer[primera_ocupada] ;
        primera_ocupada = (primera_ocupada+1) % tam_vector ;
        num_celdas_ocupadas-- ;
        cout << "Buffer va a enviar valor " << valor << endl ;
        MPI_Ssend( &valor, 1, MPI_INT, emisor, 0, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
    int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( (id_propio >= 0) && (id_propio < np) )
         funcion_productor(id_propio);
      else if ( id_propio == id_buffer )
         funcion_buffer();
      else if (id_propio > id_buffer)
         funcion_consumidor(id_propio - np);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
