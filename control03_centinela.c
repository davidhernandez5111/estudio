/*
CONTROL 03: CENTINELA - Resolucion para estudiar en lapiz y papel.

1.a) Funcion de aptitud:

    F = 0.4 * bateria + 0.4 * visibilidad + 0.2 * proximidad
    proximidad = 100 / (1 + distancia)

La distancia se ingresa en km. Los tres criterios quedan entre 0 y 100.
Bateria (40%): permite mantener al lider funcionando mas tiempo.
Visibilidad (40%): permite vigilar mejor con la camara multiespectral.
Proximidad (20%): estar cerca del perimetro facilita la vigilancia.
Estos pesos son una propuesta; el enunciado permite definirlos.

1.b) Calculos:

C1: 0.4*78 + 0.4*85 + 20/2.2 = 74.29
C2: 0.4*45 + 0.4*92 + 20/1.8 = 65.91
C4: 0.4*91 + 0.4*70 + 20/3.1 = 70.85
C5: 0.4*62 + 0.4*88 + 20/1.5 = 73.33
C6: 0.4*83 + 0.4*45 + 20/2.5 = 59.20

Orden: C1 > C5 > C4 > C2 > C6. C3 esta inoperativo y no participa.
Para simplificar el codigo, ingresamos las aptitudes ya calculadas,
redondeadas al entero mas cercano: 74, 66, 0, 71, 73, 59.
En este ejemplo el redondeo conserva el ranking. En otros ejemplos podria
generar empates; en empate se da prioridad al menor ID.

2.a) Algoritmo: eleccion de Raft con prioridad por aptitud (adaptacion).

- C1 detecta tres heartbeats consecutivos sin respuesta de C3.
- C1 avisa de la falla a los operativos y se calcula su ranking.
- El puesto 1 espera 100 ms, el puesto 2 espera 200 ms, etc.
- Vence primero el tiempo del mejor clasificado: se vuelve candidato,
  inicia una ronda (termino 1) y vota por si mismo.
- El candidato envia REQUEST_VOTE a los otros cinco drones.
- Cada operativo vota una sola vez en la ronda y responde VOTO_SI.
- Con 4 votos de los 6 integrantes originales, C1 ya tiene mayoria.
- C1 anuncia su liderazgo con HEARTBEAT; los operativos lo reconocen.

Supuestos: todos conocen la telemetria y reciben el aviso de C1 al mismo
tiempo. La votacion y el anuncio terminan antes del siguiente timeout.
No hay perdidas de mensajes ni nuevas fallas. Logs igualmente actualizados.
Se modelan mensajes mediante llamadas a funciones, sin red real.
Los tiempos de espera son virtuales: no se hace esperar al programa.
El programa representa una sola ronda, no una implementacion completa.

2.b) Gana C1: recibe 5 votos contando el suyo, superando el quorum de 4.
Ademas, C1 tiene la mayor aptitud con la formula elegida.
La aptitud determina la prioridad de candidatura; la mayoria confirma
al lider. En Raft habitual los timeouts son aleatorios: usar el ranking
es nuestra adaptacion para el ejercicio. Ante demoras o nuevas fallas,
no se garantiza que gane siempre el mejor clasificado.

Compilar: gcc -std=c11 -Wall -Wextra control03_centinela.c -o centinela
*/

#include <stdio.h>

#define N 6

int operativo[N] = {1, 1, 0, 1, 1, 1};
int voto[N] = {0};       /* 0: sin voto; otro valor: ID del candidato. */
int lider[N] = {3, 3, 3, 3, 3, 3};
int votos = 0;
int termino = 0;

/* Simula la recepcion de REQUEST_VOTE y su respuesta. */
void pedir_voto(int candidato, int destino)    {
    printf("C%d -> C%d: REQUEST_VOTE, termino %d\n",
           candidato + 1, destino + 1, termino);
    if (!operativo[destino]) {
        printf("C%d no responde: esta inoperativo.\n", destino + 1);
    } else if (voto[destino] == 0) {
        voto[destino] = candidato + 1;
        printf("C%d -> C%d: VOTO_SI\n", destino + 1, candidato + 1);
        votos++;
    }
}

/* Devuelve el indice del primer candidato, o -1 si no hay operativos.
   Mayor aptitud primero; en empate, menor ID primero. */
int priorizar_candidatos(int aptitud[]) {
    int orden[N];
    int cantidad = 0;

    /* Guardar los drones operativos. */
    for (int i = 0; i < N; i++) {
        if (operativo[i] == 1) {
            orden[cantidad] = i;
            cantidad++;
        }
    }

    /* Ordenar de mayor a menor aptitud. */
    for (int i = 0; i < cantidad - 1; i++) {
        for (int j = i + 1; j < cantidad; j++) {
            if (aptitud[orden[j]] > aptitud[orden[i]]) {
                int auxiliar = orden[i];
                orden[i] = orden[j];
                orden[j] = auxiliar;
            }
        }
    }

    /* Mostrar el ranking y la espera de cada candidato. */
    for (int i = 0; i < cantidad; i++) {
        printf("Puesto %d: C%d, aptitud %d, espera %d ms\n",
               i + 1, orden[i] + 1,
               aptitud[orden[i]], (i + 1) * 100);
    }

    if (cantidad == 0) {
        return -1;
    }

    return orden[0];
}

int main(void)
{
    /* Resultados de la formula, calculados a mano y redondeados. */
    int aptitud[N] = {74, 66, 0, 71, 73, 59};
    int fallos = 0;

    puts("APTITUD DE LOS DRONES:");
    for (int i = 0; i < N; i++) {
        if (operativo[i]) {
            printf("C%d: %d\n", i + 1, aptitud[i]);
        }
    }

    /* Tres ciclos sin respuesta: se simulan sin esperar tiempo real. */
    for (int ciclo = 1; ciclo <= 3; ciclo++) {
        puts("C1 -> C3: HB_REQ");
        if (operativo[2] == 0) {
            fallos++;
            printf("TIMEOUT: %d fallos consecutivos\n", fallos);
        } else {
            fallos = 0;
        }
    }
    if (fallos < 3) {
        return 0;
    }

    puts("C1 declara C3 inoperativo e inicia la eleccion.");
    for (int i = 1; i < N; i++)
        if (operativo[i] == 1)
            printf("C1 -> C%d: AVISO_CAIDA de C3\n", i + 1);

    int candidato = priorizar_candidatos(aptitud);
    if (candidato == -1)
        return 0;
    printf("[t=100 ms] Vence primero el tiempo de C%d.\n", candidato + 1);
    termino++;
    voto[candidato] = candidato + 1;
    votos = 1;
    printf("C%d se vuelve candidato y vota por si mismo.\n", candidato + 1);

    for (int i = 0; i < N; i++)
        if (i != candidato)
            pedir_voto(candidato, i);

    /* Mayoria de 6: 6/2 + 1 = 4. C3 sigue siendo miembro. */
    if (votos >= N / 2 + 1) {
        printf("C%d es elegido lider con %d votos.\n", candidato + 1, votos);
        lider[candidato] = candidato + 1;
        for (int i = 0; i < N; i++) {
            if (i == candidato)
                continue;
            printf("C%d -> C%d: HEARTBEAT, nuevo lider C%d\n",
                   candidato + 1, i + 1, candidato + 1);
            if (operativo[i]) {
                lider[i] = candidato + 1;
                printf("C%d reconoce a C%d como lider.\n", i + 1, lider[i]);
            }
        }
    } else {
        puts("No hay mayoria: se necesita otra ronda.");
    }
    return 0;
}
