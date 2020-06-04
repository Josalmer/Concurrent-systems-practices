// Sistemas Concurridos y Distribuidos
// ETSIIT, UGR
// Práctica 4. Implementación de sistemas de tiempo real.
// Actividad 2: nuevo ejemplo de ejecutivo ciclico
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500  100
//   B  500  150
//   C  1000  200
//   D  2000  240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D   | A B C   | A B   |
//  *---------*----------*---------*--------*
// Para compilar: g++ -std=c++11 -Wall ejecutivo2.cpp -o ejecutivo2_exe
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario
   const milliseconds Ts( 500 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();

   while( true ) // ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD();           break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB();                     break ;
         }

         // calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts ;

         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );

         // Actividad 1
         // Comprobamos el retardo y lo imprimimos por pantalla
         time_point<steady_clock> fin_real = steady_clock::now();
         steady_clock::duration retardo = fin_real - ini_sec;

         // Si es mayor que 20 abortamos
         if(milliseconds_f(retardo).count() > 20.0){
           cout << "Retardo : " << milliseconds_f(retardo).count() << " ms." << endl << " Mayor que 20 ms, abortamos ejecucion" << endl;
           exit(1);
         }
         else{
           cout << "Retardo : " << milliseconds_f(retardo).count() << " ms. " << endl;
         }
      }
   }
}

/* Respuesta a las cuestiones:
¿ cual es el mínimo tiempo de espera que queda al final de las
iteraciones del ciclo secundario con tu solución ?
Miramos el ciclo secundario más desfavorable, en nuestro caso el
segundo ciclo, en el segundo ciclo se ejecutan A, B y D. Sus
tiempos son 100 + 150 +240 = 490 ms, como cada ciclo secundario
es de 500 ms, el tiempo minimo de espera en dicho caso es de 10 ms

¿ sería planificable si la tarea D tuviese un tiempo cómputo de
250 ms ?
Si tenemos en cuenta que los tiempos C son los maximos tiempos
posibles de ejecución, y dado que ahora la peor combinación sería
la de A, B y D, con tiempos 100+150+250 = 500 ms = al tiempo de
ciclo secundario
Podemos considerar que es planificable, siempre y cuando tengamos
presente lo indicado anteriormente (C = Cmax)
*/
