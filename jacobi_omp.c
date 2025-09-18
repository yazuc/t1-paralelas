
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

void printMesh(float* meshArray, const int MESHSIZE);

int main(int argc, char** argv){
    const int MESHSIZE = 1500;                //tamanho da malha a ser computada

    const float NORTH_BOUND = 100.0;        //Valor de limite norte para a malha
    const float SOUTH_BOUND = 100.0;        //Valor de limite sul para a malha
    const float EAST_BOUND = 0.0;          //east bounding value for the mesh
    const float WEST_BOUND = 0.0;          //west bounding value for the mesh
    const float INTERIOR_AVG =              //Valor para os pontos interiores da malha
        (NORTH_BOUND + SOUTH_BOUND + EAST_BOUND + WEST_BOUND) / 4.0;    //Média dos valores de limite

    int       r;            //indice de linha para iteração
    int       c;            //indice de coluna para iteração
    int       itrCount;     //contador de iterações
    float     gDiffNorm;    //(global difference norm) representa a medida do erro entre uma iteração e a anterior
    
    float* xNew;       //matriz para armazenar os novos valores calculados
    float* xFull;      //matriz utilizada para o calculo dos novos valores

    float epsilon;          //tolerância de convergência, quando o erro fica menor que isso, o código para
    int maxIterations;      //limite máximo de iterações
    
    int reqThreads;        //número de threads a serem usadas (fornecido na linha de comando)

    double start, stop;     //variáveis para medir o tempo de execução

    //teste para garantir que o número correto de argumentos foi passado
    if (argc < 4) {
        //print usage information to head
        
        printf("Please specify the correct number of arguments.\n");
        printf("Usage: jacobi_openmp [epsilon] [max_iterations] [threads]\n");
       
        return 0;
    }

    //Alocar memória para as matrizes
    xFull = (float*)malloc(MESHSIZE * MESHSIZE * sizeof(float));
    xNew = (float*)malloc(MESHSIZE * MESHSIZE * sizeof(float));

    //popular variáveis a partir da linha de comando
    epsilon = strtod(argv[1], NULL);
    maxIterations = strtol(argv[2], NULL, 10);
    reqThreads = strtol(argv[3], NULL, 10);


    /* Inicializando as matrizes */
    for (r = 0; r < MESHSIZE; r++) { //começa preenchendo o interior e os limites leste e oeste
        for (c = 0; c < MESHSIZE; c++) { //passa por todas as posições
            xFull[r * MESHSIZE + c] = INTERIOR_AVG;    //preenche o interior com a média dos valores de limite
        }

        xFull[r * MESHSIZE + MESHSIZE - 1] = EAST_BOUND; //inicializa o valor do limite leste (ultima coluna)
        xFull[r * MESHSIZE + 0] = WEST_BOUND;          //inicializa o valor do limite oeste (primeira coluna)
    }
    for (c = 0; c < MESHSIZE; c++) {
        xFull[0 * MESHSIZE + c] = NORTH_BOUND;   //inicializa o valor do limite norte (primeira linha (0))
        xFull[(MESHSIZE - 1) * MESHSIZE + c] = SOUTH_BOUND;    //inicializa o valor do limite sul (ultima linha (MESHSIZE-1))
    }

    for (r = 0; r < MESHSIZE; r++) {  //copia os valores iniciais de xFull para xNew
        for (c = 0; c < MESHSIZE; c++) {
            xNew[r * MESHSIZE + c] = xFull[r * MESHSIZE + c];
        }
    }


    start = omp_get_wtime(); //inicia o timer

    //loop principal de iteração
    itrCount = 0;   //zera o contador de iterações

    do { //laço do...while para as iterações de Jacobi

        itrCount++; //incrementa o contador de iterações
        gDiffNorm = 0.0; //zera o gDiffNorm para a iteração atual


        //divide o trabalho entre as threads, cada thread calcula uma linha da matriz, cada thread tem sua própria cópia de r e c
        //e o gDiffNorm é reduzido somando os valores de cada thread no final
#pragma omp parallel for private(r,c) reduction(+:gDiffNorm) num_threads(reqThreads)
            for (r = 1; r < MESHSIZE - 1; r++) //percorre todas as linhas, exceto os limites para manter as bordas fixas
                for (c = 1; c < MESHSIZE - 1; c++) { //percorre todas as colunas, exceto os limites para manter as bordas fixas
                    xNew[r * MESHSIZE + c] = (xFull[r * MESHSIZE + c + 1] + xFull[r * MESHSIZE + c - 1] +     //calcula o novo valor como a média dos 4 vizinhos
                        xFull[(r + 1) * MESHSIZE + c] + xFull[(r - 1) * MESHSIZE + c]) / 4.0;

                    //armazena o quadrado da diferença entre o novo e o antigo valor para calcular o erro normalizado
                    gDiffNorm += (xNew[r * MESHSIZE + c] - xFull[r * MESHSIZE + c]) *           
                        (xNew[r * MESHSIZE + c] - xFull[r * MESHSIZE + c]);
                }
        

        //uma vez que foi calculado o novo valor para cada célula, trocamos os ponteiros para que xFull aponte para a matriz com os novos valores
        float* tmp = xFull;
        xFull = xNew;
        xNew = tmp;
        
        gDiffNorm = sqrt(gDiffNorm);  //terminamos o cálculo do gDiffNorm tirando a raiz quadrada da soma dos quadrados das diferenças
        
    } while (gDiffNorm > epsilon && itrCount < maxIterations);  //keep doing Jacobi iterations until we hit our iteration or precision limit


    stop = omp_get_wtime(); //stop the timer

    printf("%d Jacobi iterations took %f seconds.\n", itrCount, stop - start);

    // writeToPPM(xFull, itrCount, MESHSIZE);

    //free our dynamic memory
    free(xNew);
    free(xFull);

    //display normal termination message and exit
    printf("<normal termination>\n");
    return 0;
}

//print contents of 2d array to console (for testing purposes)
void printMesh(float* meshArray, const int MESHSIZE) {
    int r, c;   //loop control variables

    for (r = 0; r < MESHSIZE; r++) {
        for (c = 0; c < MESHSIZE; c++) {
            //print cell with width 5 and 1 digit after the decimal
            printf("%5.1f ", meshArray[r * MESHSIZE + c]);
        }
        //print newline
        printf("\n");
    }
}