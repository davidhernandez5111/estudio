#include <stdio.h>
#include <stdbool.h>

#define CANT_DRONES 6
#define MAX_CICLOS 3
#define QUORUM ((CANT_DRONES / 2) + 1)

typedef enum {
    FOLLOWER,
    CANDIDATE,
    LEADER
} EstadoRaft;

typedef struct {
    int id;

    EstadoRaft estado;
    int terminoactual;
    int votadopor;
    int votosrecibidos;

    bool operativo;
    int ciclosSinLatido;

    int bateria;
    int visibilidad;
    float distancia;
} Node;

typedef struct {
    int candidateId;
    int term;
    float aptitud;
} Msg;

/* 1. CALCULAR APTITUD */
float calcularAptitud(Node nodo) {
    return nodo.bateria * 0.5f + nodo.visibilidad * 0.3f - nodo.distancia * 10.0f;
}

/* 2. PROCESAR VOTOS */
int procesar_votos(Node enjambre[], int i, Msg msg) {
    if (enjambre[i].operativo == false) {
        return 0;
    }

    // Actualizar el termino si llega uno mayor.
    if (msg.term > enjambre[i].terminoactual) {
        enjambre[i].terminoactual = msg.term;
        enjambre[i].votadopor = -1;
        enjambre[i].estado = FOLLOWER;
    }

    // Comprobar si puede conceder el voto.
    if (msg.term == enjambre[i].terminoactual && (enjambre[i].votadopor == -1 || enjambre[i].votadopor == msg.candidateId) && msg.aptitud > calcularAptitud(enjambre[i])) {
        enjambre[i].votadopor = msg.candidateId;
        printf("C%d: OTORGA voto a C%d, termino %d\n", enjambre[i].id, msg.candidateId, msg.term);
        return 1;
    }
    printf("C%d: DENIEGA voto a C%d, termino %d\n", enjambre[i].id, msg.candidateId, msg.term);
    return 0;
}

/* 3. INICIAR ELECCION */

// Devuelve el indice del mejor operativo, o -1 si no queda ninguno.
int mejor_operativo(Node enjambre[]) {
    int mejor = -1;
    for (int i = 0; i < CANT_DRONES; i++) {
        if (enjambre[i].operativo) {
            if (mejor == -1 ||
                calcularAptitud(enjambre[i]) > calcularAptitud(enjambre[mejor]) ||
                (calcularAptitud(enjambre[i]) == calcularAptitud(enjambre[mejor]) &&
                 enjambre[i].id < enjambre[mejor].id)) {
                mejor = i;
            }
        }
    }
    return mejor;
}

// Variante sin voto propio: esta regla difiere de Raft.
void iniciar_eleccion(Node enjambre[], int candidato) {
    while (candidato != -1) {
        if (!enjambre[candidato].operativo) {
            enjambre[candidato].estado = FOLLOWER;
            candidato = mejor_operativo(enjambre);
            continue;
        }

        int termino = 0;
        for (int i = 0; i < CANT_DRONES; i++) {
            if (enjambre[i].terminoactual > termino) {
                termino = enjambre[i].terminoactual;
            }
        }

        enjambre[candidato].estado = CANDIDATE;
        enjambre[candidato].terminoactual = termino + 1;
        enjambre[candidato].votadopor = -1;
        enjambre[candidato].votosrecibidos = 0;

        Msg msg = {
            .candidateId = enjambre[candidato].id,
            .term = enjambre[candidato].terminoactual,
            .aptitud = calcularAptitud(enjambre[candidato])
        };
        printf("\nC%d inicia eleccion, termino %d\n", msg.candidateId, msg.term);

        // operativo debe actualizarse al detectar una desconexion.
        // Este programa secuencial no detecta por si solo una falla de red.
        for (int i = 0; i < CANT_DRONES; i++) {
            if (!enjambre[candidato].operativo) {
                break;
            }
            if (i != candidato) {
                // procesar_votos devuelve 0 si el receptor esta inoperativo.
                enjambre[candidato].votosrecibidos += procesar_votos(enjambre, i, msg);
            }
        }

        // Comprobar tambien antes de declarar al ganador.
        if (!enjambre[candidato].operativo) {
            enjambre[candidato].estado = FOLLOWER;
            printf("C%d no esta operativo. Se inicia otro termino.\n", msg.candidateId);
            candidato = mejor_operativo(enjambre);
            continue;
        }

        printf("C%d obtuvo %d votos; necesita %d.\n",
               msg.candidateId, enjambre[candidato].votosrecibidos, QUORUM);
        if (enjambre[candidato].votosrecibidos >= QUORUM) {
            enjambre[candidato].estado = LEADER;
            printf("C%d asume el liderazgo.\n", msg.candidateId);
        } else {
            printf("Sin quorum. No se elige lider en este intento.\n");
        }
        return;
    }
    printf("No quedan candidatos operativos.\n");
}

/* 4. MONITOREAR HEARTBEATS */

void monitor_heartbeats(Node enjambre[], int observador, int id_lider) {
    if (enjambre[observador].estado == LEADER) {
        return;
    }

    for (int i = 0; i < CANT_DRONES; i++) {
        if (enjambre[i].id == id_lider &&
            enjambre[i].operativo) {

            // Conservamos la simulacion del codigo original.
            bool heartbeat_recibido = false;

            if (heartbeat_recibido) {
                enjambre[i].ciclosSinLatido = 0;
            } else {
                enjambre[i].ciclosSinLatido++;

                printf("[HB] C%d sin respuesta. Ciclos: %d\n",
                       id_lider,
                       enjambre[i].ciclosSinLatido);

                if (enjambre[i].ciclosSinLatido >= MAX_CICLOS) {
                    printf("C%d declarado inoperativo.\n",
                           id_lider);

                    enjambre[i].operativo = false;

                    iniciar_eleccion(enjambre, observador);
                }
            }
        }
    }
}

/* 5. MAIN */

int main(void) {
    Node enjambre[CANT_DRONES] = {
        // ID, estado, termino, voto, votos recibidos,
        // operativo, fallos, bateria, visibilidad, distancia
        {1, FOLLOWER, 1, -1, 0, true, 0, 78, 85, 1.2f},
        {2, FOLLOWER, 1, -1, 0, true, 0, 45, 92, 0.8f},
        {3, LEADER,   1, -1, 0, true, 0, 95, 90, 0.0f},
        {4, FOLLOWER, 1, -1, 0, true, 0, 91, 70, 2.1f},
        {5, FOLLOWER, 1, -1, 0, true, 0, 62, 88, 0.5f},
        {6, FOLLOWER, 1, -1, 0, true, 0, 83, 45, 1.5f}
    };

    printf("--- SIMULACION: CAIDA DE C3 ---\n");

    for (int ciclo = 0; ciclo < MAX_CICLOS; ciclo++) {
        // El indice 0 corresponde a C1, que vigila a C3.
        monitor_heartbeats(enjambre, 0, 3);
    }

    return 0;
}