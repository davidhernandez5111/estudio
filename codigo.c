#define Numero 6

int drones[Numero] = {1,1,0,1,1,1}; // los drones, definimos los activos y los inactivos
int voto[Numero] = {0}; // Acá se almacena por quien votó cada dron
int lider[Numero] = {3,3,3,3,3,3}; // En el punto inicial, todos asumen que el lider es el dron N3.
int votos = 0; // Contador de votos
int termino = 0; // Contador de terminos

void pedir_voto(int candidato, int destino){
    printf("C%d -> C%d: REQUEST_VOTE, termino %d\n", candidato+1, destino+1, termino); // printeamos la operacion de solicitud de voto
    
    if (drones[destino] == 0){
        printf("C%d no responde, está inactivo", destino+1);
    }

    else if (voto[destino] == 0){
        voto[destino] = candidato + 1; // guardamos el voto por quien votó hacia el que votó
        printf("C%d -> C%d: VOTO_SI\n", destino+1, candidato+1); // printeamos la respuesta del voto
        votos++; // aumentamos el contador de votos, indicando que el dron ya votó
    }

}

int priorizar_candidatos(int aptitud[]){
    int orden[Numero];
    int cantidad = 0;

    /* Guardar los drones operativos. */
    for (int i = 0; i < Numero; i++) {
        if (drones[i] == 1) {
            orden[cantidad] = i;
            cantidad++;
        }
    }

    /* Ordenar de mayor a menor aptitud. */
    for (int i = 0; i < cantidad - 1; i++) {
        for (int j = i+1; j<cantidad; j++){
            if (aptitud[orden[j]] > aptitud[orden[i]]){
                int auxiliar = orden[i]; // esto simplemente es un auxiliar q nos da un espacio para dejar el valor de orden[i] y poder hacer el intercambio
                orden[i] = orden[j];
                orden[j] = auxiliar;
            }
        }
    }

    for (int i = 0; i < cantidad; i++) {
        printf("Puesto %d: C%d, aptitud %d\n", i + 1, orden[i] + 1, aptitud[orden[i]]);
    }

    return orden[0];
}

int main(void){
    int aptitud[Numero] = {74, 66, 9, 71, 23, 39};
    int fallos = 0;

    for (int i=0; i<Numero; i++){
        if (drones[i] == 1){
            printf("C%d: %d\n", i+1, aptitud[i]);     //declaramos los drones operativos
        }
    }

    for (int i=0; i<=3; i++){
        if (drones[2] == 0){    // le hacemos heartbeat al dron 3 veces, si a la 3ra no responde lo declaramos como inoperativo
            fallos++;
        }
        else {
            fallos = 0;    // en el caso q responda, volvemos al contador en 0
        }
    }
    
    if (fallos < 3){
        return 0;
    }

    printf("Dron C3 se decalra inoerativo, se inicia la seleccion de lider");
    for (int i=0; i<Numero; i++){
        if (drones[i] == 1){
            printf("El dron C3 se ha caído, se le notifica al dron C%d", i+1);
        }
    }

    /* 1. Obtener al primero del ranking. */
    int candidato = priorizar_candidatos(aptitud);

    if (candidato == -1) {
        printf("No hay drones operativos.\n");
        return 0;
    }

    /* 2. Iniciar la ronda. */
    termino++;
    votos = 0;

    printf("C%d se presenta como candidato.\n", candidato + 1);

    /* 3. Solicitar votos a todos los demas. */
    for (int i = 0; i < Numero; i++) {
        if (i != candidato) {
            pedir_voto(candidato, i);
        }
    }

    /* 4. Comprobar la mayoria. */
    int mayoria = Numero / 2 + 1;

    if (votos >= mayoria) {
        printf("C%d gana con %d votos.\n", candidato + 1, votos);

        lider[candidato] = candidato + 1;

        /* 5. Anunciar al nuevo lider. */
        for (int i = 0; i < Numero; i++) {
            if (i != candidato && drones[i] == 1) {
                printf("C%d -> C%d: HEARTBEAT, nuevo lider C%d\n", candidato + 1, i + 1, candidato + 1);
                lider[i] = candidato + 1;
            }
        }
    } else {
        printf("No hay mayoria. Se necesita otra ronda.\n");
    }

    return 0;
}