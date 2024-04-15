/*
Autor: Andre Franco
Descricao: Este programa efetua a gestao de voos e aeroportos
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Numero maximo de Aeroportos*/
#define Aeroporto 40
/*Numero maximo de Voos*/
#define Voos 30000
/*Comprimento de uma string que define o ID de um aeroporto*/
#define IDAeroporto 5
/*Comprimento maximo de uma string que define o pais de um aeroporto*/
#define Pais 30
/*Comprimento maximo de uma string que define a cidade de um aeroporto*/
#define Cidade 50
/*Max_Str_Length: Tamanho maximo do input da funcao lista_voos*/
#define Max_Str_Length 42
/*Comprimento maximo de uma string que define o codigo de um voo*/
#define Cod_Voo 6
/*Tipos diferentes que caracterizam uma data*/
#define Datas_Voo 3
/*Tipos diferentes que caracterizam uma hora*/
#define Horas_Voo 2
/*Meses do ano*/
#define Meses 12
/*Tipos de Voo(Chegada ou Partida)*/
#define Chegada_Partida 2

/*Lista que contem os dias que cada mes tem*/
int dias_do_mes[Meses] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
/*Array que define a data do sistema*/
int data[Datas_Voo + 1] = {1, 1, 2022};
/*Lista em que esta guardado o ID de cada aeroporto criado*/
char IDaeroporto[Aeroporto][IDAeroporto + 1];
/*Lista em que esta guardado o pais de cada aeroporto criado*/
char pais[Aeroporto][Pais + 1];
/*Lista em que esta guardado a cidade de cada aeroporto criado*/
char cidade[Aeroporto][Cidade + 1];
int num_voos[Aeroporto];
/*Lista em que esta guardado o ID de Voo de cada voo criado*/
char IDvoos[Voos][Cod_Voo + 1];
/*Lista em que esta guardado o aeroporto de partida de cada voo criado*/
char AeroPart[Voos][IDAeroporto + 1];
/*Lista em que esta guardado o aeroporto de chegada de cada voo criado*/
char AeroCheg[Voos][IDAeroporto + 1];
/*Array que guarda as datas de chegada(Datas[Voos][0]) 
e de partida(Datas[Voos][1]) por ordem*/
int Datas[Voos][Chegada_Partida][Datas_Voo];
/*Array que guarda as horas de chegada(Horas[Voos][0])
e de partida(Horas[Voos][1]) por ordem*/
int Horas[Voos][Chegada_Partida][Horas_Voo];
/*Array para guardar o indice 
original dos voos criados*/
int indices[Voos];
/*Array que guarda o numero de passageiros de um voo*/
int passageiros[Voos];
/*Inteiro que guarda o numero de aeroportos criados*/
int contador_aeroportos = 0;
/*Inteiro que guarda o numero de voos criados*/
int contador_voos = 0;
/*Inteiro que guarda o numero de reservas criadas*/
int contador_reservas = 0;

typedef struct{
    char* str_reservas;
    char cod_voo[Cod_Voo + 1];
    int indice_voo;
    int num_pass;
} reservas;

/*Array que guarda reservas criadas*/
reservas* reservas_arr;

int leProximaPalavra(char str[]) {
	char c = getchar();
	int i = 0;
	while (c == ' ' || c == '\t')
		c = getchar();
	while (c != ' ' && c != '\t' && c != '\n') {
		str[i++] = c;
		c = getchar();
	}
	str[i] = '\0';
	return (c == '\n');
}

/*Verifica se o aeroporto foi criado*/
int verifica_aeroporto_criado(char ID[], int k){
    int i;
    for(i = 0; i < contador_aeroportos; i++){
        if(strcmp(IDaeroporto[i], ID) == 0){
            if(k == 0){
                printf("duplicate airport\n");
                return 0;
            }
            else{
                return i;
            }
        }
    }
    if(k != 0){
        printf("%s: no such airport ID\n", ID);
        return -1;
    }
    return 1;
}

/*Verifica os parametros da funcao cria_aeroporto*/
int verifica_aeroporto(char ID[]){
    int i, x;
    if(contador_aeroportos >= Aeroporto){
        printf("too many airports\n");
        return 0;
    }
    if((x = strlen(ID)) < 3 || strlen(ID) > 5){
        printf("invalid airport ID\n");
        return 0;
    }
    for(i = 0; i < x; i++){
        if((ID[i] < 65 || ID[i] > 90) && (ID[i] < 97 || ID[i] > 122)){
            printf("invalid airport ID\n");
            return 0;
        }
    }
    if(verifica_aeroporto_criado(ID, 0) == 0){
        return 0;
    }
    return 1;
}

/*Cria um aeroporto se cumprir certos parametros
e guarda essas informacoes numa lista*/
int cria_aeroporto(){
    char ID[IDAeroporto + 1], pais_lcl[Pais + 1], cidade_lcl[Cidade + 1];
    scanf(" %s %s %[^\n]s", ID, pais_lcl, cidade_lcl);
    /*Verifica os parametros introduzidos*/
    if(verifica_aeroporto(ID) == 0){
        return 0;
    }
    /*Guarda as informacoes do voo*/
    num_voos[contador_aeroportos] = 0;
    strcpy(IDaeroporto[contador_aeroportos], ID);
    strcpy(pais[contador_aeroportos], pais_lcl);
    strcpy(cidade[contador_aeroportos], cidade_lcl);
    contador_aeroportos++;
    printf("airport %s\n", ID);
    return 0;
}

int compara_apenas_datas(int data1[], int data2[]){
    if(data1[2] > data2[2]){
        return 1;
    }
    else if(data1[1] > data2[1] && data1[2] == data2[2]){
        return 1;
    }
    else if(data1[0] > data2[0] && data1[1] == data2[1]){
        return 1;
    }
    else{
        return 0;
    }
}

/*Compara duas datas e retorna 1 se a data 1 foi posterior a data 2*/
int compara_datas(int data1[], int hora1[], int data2[], int hora2[]){
    if(data1[2] > data2[2]){
        return 1;
    }
    else if(data1[1] > data2[1] && data1[2] == data2[2]){
        return 1;
    }
    else if(data1[0] > data2[0] && data1[1] == data2[1]){
        return 1;
    }
    else if(hora1[0] > hora2[0] && data1[0] == data2[0]){
        return 1;
    }
    else if(hora1[1] > hora2[1] && hora1[0] == hora2[0]){
        return 1;
    }
    return 0;
}

/*Ordena os aeroportos criados por ordem alfabetica*/
int bubble_sort_l(){
    int i, j;
    char temp_id[IDAeroporto + 1];
    char temp_pais[Pais + 1];
    char temp_cidade[Cidade + 1];
    int temp_num_voos;
    for(i = 0; i < contador_aeroportos; i++){
        for(j = 0; j < contador_aeroportos - i - 1; j++){
            if(strcmp(IDaeroporto[j], IDaeroporto[j+1]) > 0){
                strcpy(temp_id, IDaeroporto[j]);
                strcpy(IDaeroporto[j], IDaeroporto[j+1]);
                strcpy(IDaeroporto[j+1], temp_id);
                strcpy(temp_pais, pais[j]);
                strcpy(pais[j], pais[j+1]);
                strcpy(pais[j+1], temp_pais);
                strcpy(temp_cidade, cidade[j]);
                strcpy(cidade[j], cidade[j+1]);
                strcpy(cidade[j+1], temp_cidade);
                temp_num_voos = num_voos[j];
                num_voos[j] = num_voos[j+1];
                num_voos[j+1] = temp_num_voos;
            }
        }
    }
    for(i = 0; i < contador_aeroportos; i++){
        printf("%s %s %s %d\n", IDaeroporto[i], cidade[i], pais[i],
        num_voos[i]);
    }
    return 0;
}

/*Recebe uma string e determina o ID de aeroportos nessa string 
e devolve os parametros relativos a um aeroporto*/
int ordena_aeroportos(char* string){
    int i, x, y, c, k;
    char lst[Aeroporto][IDAeroporto + 1];
    x = strlen(string);
    y = 0;
    /*(x/(IDAeroporto + 1)) calcula o numero maximo
    de aeroportos que podem estar na string*/
    for(i = 0; i < (x/(IDAeroporto + 1)); i++){
        /*k e uma funcao que calcula onde estao
        os ID dos Aeroportos na string*/
        k = (IDAeroporto + 1)*i + 1;
        lst[i][3] = '\0';
        lst[i][2] = string[k + 2];
        lst[i][1] = string[k + 1];
        lst[i][0] = string[k];
        y++;
    }
    for(i = 0; i < y; i++){
        if((c = verifica_aeroporto_criado(lst[i], 1)) == -1){
            continue;
        }
        printf("%s %s %s %d\n", IDaeroporto[c], cidade[c], pais[c], num_voos[c]);
    }
    return 0;
}

/*Se conter parametros lista os aeroportos introduzidos
senao apenas ordena todos os aeroportos criados*/
int lista_aeroportos(){
    char string[(Aeroporto*(IDAeroporto + 1)) + 1];
    /*Verifica se a funcao tem parametros ou nao*/
    if(scanf("%[^\n]s", string) == 1){
        ordena_aeroportos(string);
    }
    else{
        bubble_sort_l();
        return 0;
    }
    return 0;
}

/*Verifica se a data introduzida e uma data valida*/
int ver_data(int dia, int mes, int ano){
    if(dia < data[0] && mes <= data[1] && ano <= data[2]){
        printf("invalid date\n");
        return 1;
    }
    else if(mes < data[1] && ano == data[2]){
        printf("invalid date\n");
        return 1;
    }
    else if((ano > data[2] && mes > data[1]) || (ano > data[2] && mes == data[1]
     && dia > data[0])|| ano < data[2]){
        printf("invalid date\n");
        return 1;
    }
    return 0;
}

/*Calcula a data e hora de chegada e guarda as 
datas e horas de chegada e de partida numa lista*/
int datas_voo(int dia, int mes, int ano, int horapart, 
int minpart, int durhora, int durmin){
    int horas_lcl[Chegada_Partida][Horas_Voo + 1], 
    datas_lcl[Chegada_Partida][Datas_Voo + 1], i, j;
    horas_lcl[0][0] = horapart + durhora;
    horas_lcl[1][0] = horapart;
    horas_lcl[0][1] = minpart + durmin;
    horas_lcl[1][1] = minpart;
    for(j = 0; j < Chegada_Partida; j++){
        datas_lcl[j][0] = dia;
        datas_lcl[j][1] = mes;
        datas_lcl[j][2] = ano;
    }
    if(horas_lcl[0][1] >= 60){
        horas_lcl[0][0] = horas_lcl[0][0] + 1;
        horas_lcl[0][1] = horas_lcl[0][1] - 60;
    }
    if(horas_lcl[0][0] >= 24){
        if(datas_lcl[0][0] < dias_do_mes[mes-1]){
            datas_lcl[0][0] = datas_lcl[0][0] + 1;
            horas_lcl[0][0] = horas_lcl[0][0] - 24;
        }
        else if((datas_lcl[0][0] >= dias_do_mes[mes-1]) && (mes-1 <= 10)){
            datas_lcl[0][0] = 1;
            datas_lcl[0][1] = datas_lcl[0][1] + 1;
            horas_lcl[0][0] = horas_lcl[0][0] - 24;
        }
        else{
            datas_lcl[0][0] = 1;
            datas_lcl[0][1] = 1;
            datas_lcl[0][2] = datas_lcl[0][2] + 1;
            horas_lcl[0][0] = horas_lcl[0][0] - 24;
        }
    }
    for(j = 0; j < Chegada_Partida; j++){
        i = contador_voos;
        Datas[i][j][0] = datas_lcl[j][0];
        Datas[i][j][1] = datas_lcl[j][1];
        Datas[i][j][2] = datas_lcl[j][2];
        Horas[i][j][0] = horas_lcl[j][0];
        Horas[i][j][1] = horas_lcl[j][1];
    }
    return 0;
}

/*Verifica se um voo ja foi criado*/
int verifica_voo_criado(char ID[], int dia, int mes, int ano, int k){
    int i;
    for(i = 0; i < contador_voos; i++){
        if((strcmp(IDvoos[i], ID) == 0) && (dia == Datas[i][1][0] && 
        mes == Datas[i][1][1] && ano == Datas[i][1][2])){
            if(k == 0){
                printf("flight already exists\n");
                return 0;
            }
            else{
                return i;
            }
        }
    }
    if(k != 0){
        printf("%s: flight does not exist\n", ID);
        return -1;
    }
    return 1;
}

/*Verifica os parametros da funcao verifica_voo*/
int verifica_voo(char ID[], int dia, int mes, int ano){
    int i, x;
    x = strlen(ID);
    for(i = 0; i < x; i++){
        if(i < 2 && !(ID[i] >= 65 && ID[i] <= 90)){
            printf("invalid flight code\n");
            return 0;
        }
        else if(i == 2 && !(ID[i] >= 49 && ID[i] <= 57)){
            printf("invalid flight code\n");
            return 0;
        }
        else if(i > 2 && !((ID[i] >= 48 && ID[i] <= 57)
        || (ID[i] == '\0'))){
            printf("invalid flight code\n");
            return 0;
        }
    }
    if(ver_data(dia, mes, ano) == 1){
        return 0;
    }
    if(verifica_voo_criado(ID, dia, mes, ano, 0) == 0){
        return 0;
    }
    return 1;
}

/*Se conter parametros cria um voo se ele cumprir os
parametros necessarios e guarda as informacoes recebidas
senao lista os voos criados por ordem*/
int lista_voos(){
    int i, c, dia, mes, ano, horapart, minpart, durhora, durmin, capacidade;
    char IDVoo_lcl[Cod_Voo + 1], IDAeroPart_lcl[IDAeroporto + 1],
    IDAeroCheg_lcl[IDAeroporto + 1], string[Max_Str_Length + 1];
    if(scanf("%[^\n]s", string) == 1){
        sscanf(string, " %s %s %s %d-%d-%d %d:%d %d:%d %d", IDVoo_lcl, 
        IDAeroPart_lcl, IDAeroCheg_lcl, &dia, &mes, &ano, &horapart,
        &minpart, &durhora, &durmin, &capacidade);
        /*Verifica as informacoes introduzidas*/
        if(contador_voos >= Voos){
            printf("too many flights\n");
            return 0;
        }
        if((c = verifica_aeroporto_criado(IDAeroPart_lcl, 1)) == -1 || 
        verifica_aeroporto_criado(IDAeroCheg_lcl, 1) == -1){
            return 0;
        }
        if(verifica_voo(IDVoo_lcl, dia, mes, ano) == 0){
            return 0;
        }
        if(durhora > 12 || (durhora == 12 && durmin > 0)){
            printf("invalid duration\n");
            return 0;
        }
        if(capacidade < 10){
            printf("invalid capacity\n");
            return 0;
        }
        i = contador_voos;
        strcpy(IDvoos[i], IDVoo_lcl);
        strcpy(AeroPart[i], IDAeroPart_lcl);
        strcpy(AeroCheg[i], IDAeroCheg_lcl);
        datas_voo(dia, mes, ano, horapart, minpart, durhora, durmin);
        passageiros[i] = capacidade;
        contador_voos++;
        return 0;
    }
    else{
        /*Ordena os voos por ordem de criacao*/
        for(i = 0; i < contador_voos; i++){
            printf("%s %s %s %02d-%02d-%d %02d:%02d\n", IDvoos[i], AeroPart[i], 
            AeroCheg[i], Datas[i][1][0], Datas[i][1][1], 
            Datas[i][1][2], Horas[i][1][0], Horas[i][1][1]);
        }
        return 0;
    }
    return 0;
}

/*Verifica se um aeroporto e um aeroporto de partida ou chegada*/
int part_cheg(char lst[], char lst2[][IDAeroporto + 1]){
    int i;
    for(i = 0; i < contador_voos; i++){
        if((strcmp(lst, lst2[i]) == 0) && strcmp(lst, "") != 0){
            return 0;
        }
    }
    return 1;
}

/*Funcao que organiza as datas e horas de forma mais antiga
para mais recente dependendo do parametro introduzido
1 para partidas e 0 para chegadas*/
int bubble_sort_geral(int k){
    int i, j;
    int temp;
    int data_lcl[Voos][Datas_Voo + 1];
    int hora_lcl[Voos][Horas_Voo + 1];
    int temp_data[Datas_Voo + 1];
    int temp_hora[Horas_Voo + 1];
    for(i = 0; i < contador_voos; i++){
        indices[i] = i;
        data_lcl[i][0] = Datas[i][k][0];
        data_lcl[i][1] = Datas[i][k][1];
        data_lcl[i][2] = Datas[i][k][2];
        hora_lcl[i][0] = Horas[i][k][0];
        hora_lcl[i][1] = Horas[i][k][1];
    }
    for(i = 0; i < contador_voos; i++){
        for(j = 0; j < contador_voos - i - 1; j++){
            if(compara_datas(data_lcl[j], hora_lcl[j], data_lcl[j+1], 
            hora_lcl[j+1]) == 1){
                temp = indices[j];
                indices[j] = indices[j+1];
                indices[j+1] = temp;
                temp_data[0] = data_lcl[j][0];
                temp_data[1] = data_lcl[j][1];
                temp_data[2] = data_lcl[j][2];
                temp_hora[0] = hora_lcl[j][0];
                temp_hora[1] = hora_lcl[j][1];
                data_lcl[j][0] = data_lcl[j+1][0];
                data_lcl[j][1] = data_lcl[j+1][1];
                data_lcl[j][2] = data_lcl[j+1][2];
                hora_lcl[j][0] = hora_lcl[j+1][0];
                hora_lcl[j][1] = hora_lcl[j+1][1];
                data_lcl[j+1][0] = temp_data[0];
                data_lcl[j+1][1] = temp_data[1];
                data_lcl[j+1][2] = temp_data[2];
                hora_lcl[j+1][0] = temp_hora[0];
                hora_lcl[j+1][1] = temp_hora[1];
            }
        }
    }
    return 0;
}

/*Le o aeroporto de partida e chama a funcao que da sort*/
int lista_partidas(){
    int i;
    char IDAeroPart_lcl[IDAeroporto + 1];
    scanf(" %s", IDAeroPart_lcl);
    if(verifica_aeroporto_criado(IDAeroPart_lcl, 1) == -1){
        return 0;
    }
    bubble_sort_geral(1);
    for(i = 0; i < contador_voos; i++){
        int x = indices[i];
        if(strcmp(IDAeroPart_lcl, AeroPart[x]) == 0 && Datas[x][1][0] == data[0] 
        && Datas[x][1][1] == data[1] && Datas[x][1][2] == data[2]){
            printf("%s %s %02d-%02d-%d %02d:%02d\n", IDvoos[x], AeroCheg[x],
            Datas[x][1][0], Datas[x][1][1], Datas[x][1][2],Horas[x][1][0], Horas[x][1][1]);
        }
    }
    return 0;
}

/*Le o aeroporto de chegada e chama a funcao que da sort*/
int lista_chegadas(){
    int i;
    char IDAeroCheg_lcl[IDAeroporto + 1];
    scanf(" %s", IDAeroCheg_lcl);
    if(verifica_aeroporto_criado(IDAeroCheg_lcl, 1) == -1){
        return 0;
    }
    bubble_sort_geral(0);
    for(i = 0; i < contador_voos; i++){
        int x = indices[i];
        if(strcmp(IDAeroCheg_lcl, AeroCheg[x]) == 0){
            printf("%s %s %02d-%02d-%d %02d:%02d\n", IDvoos[x], AeroPart[x],
            Datas[x][0][0], Datas[x][0][1], Datas[x][0][2],Horas[x][0][0], Horas[x][0][1]);
        }
    }
    return 0;
}

/*Avanca a data se a data introduzida for valida*/
int avanca_data(){
    char func;
    int dia, mes, ano;
    scanf("%c %d-%d-%d", &func, &dia, &mes, &ano);
    if(ver_data(dia, mes, ano) == 1){
        return 0;
    }
    data[0] = dia;
    data[1] = mes;
    data[2] = ano;
    printf("%02d-%02d-%d\n", dia, mes, ano);
    return 0;
}

/*Da sort ao array onde estao as reservas*/
int bubble_sort_reservas(char cod_voo[], int dia, int mes, int ano){
    int i, j;
    reservas temp;
    for(i = 0; i < contador_reservas; i++){
        for(j = 0; j < contador_reservas - i - 1; j++){
            if(strcmp(reservas_arr[j].str_reservas, reservas_arr[j+1].str_reservas) > 0){
                temp = reservas_arr[j];
                reservas_arr[j] = reservas_arr[j+1];
                reservas_arr[j+1] = temp;
            }
        }
    }
    for(i = 0; i < contador_reservas; i++){
        if(strcmp(IDvoos[reservas_arr[i].indice_voo], cod_voo) == 0 
        && verifica_voo_criado(IDvoos[reservas_arr[i].indice_voo], dia, mes, ano, 1) 
        == reservas_arr[i].indice_voo){
            printf("%s %d\n", reservas_arr[i].str_reservas, reservas_arr[i].num_pass);
        }
    }
    return 0;
}

/*Verifica os parametros introduzidos na funcao cria_reserva*/
int verifica_reservas(reservas r, int x){
    int i;
    for(i = 0; i < x; i++){
        if(x < 10){
            printf("invalid reservation code\n");
            return 0;
        }
        if(!((r.str_reservas[i] >= 65 && r.str_reservas[i] <= 90) ||
        (r.str_reservas[i] >= 48 && r.str_reservas[i] <= 57))){
            printf("invalid reservation code\n");
            return 0;
        }
    }
    if(r.num_pass <= 0){
        printf("invalid passenger number\n");
        return 0;
    }
    if(r.num_pass > passageiros[r.indice_voo]){
        printf("too many reservations\n");
        return 0;
    }
    for(i = 0; i < contador_reservas; i++){
        if(strcmp(reservas_arr[i].str_reservas, r.str_reservas) == 0){
        printf("%s: flight reservation already used\n", r.str_reservas);
            return 0;
        }
    }
    return 1;
}

int cria_reserva(){
    reservas r;
    char* str_num_pass;
    int dia, mes, ano, x, i;
    r.str_reservas = (char*)malloc(sizeof(int)*sizeof(r.str_reservas));
    if(r.str_reservas == NULL){
        printf("No memory\n");
        for(i = 0; i < contador_reservas; i++){
            free(reservas_arr[i].str_reservas);
        }
        free(reservas_arr);
        exit(1);
    }
    scanf(" %s %d-%d-%d", r.cod_voo, &dia, &mes, &ano);
    leProximaPalavra(r.str_reservas);
    x = strlen(r.str_reservas);
    if((r.indice_voo = verifica_voo_criado(r.cod_voo, dia, mes, ano, 1)) == -1){
        return 0;
    }
    if(ver_data(dia, mes, ano) == 1){
        return 0;
    }
    if(x != 0){
        str_num_pass = (char*)malloc(sizeof(int)*sizeof(str_num_pass));
        if(str_num_pass == NULL){
            printf("No memory\n");
            free(r.str_reservas);
            for(i = 0; i < contador_reservas; i++){
                free(reservas_arr[i].str_reservas);
            }
            free(reservas_arr);
            exit(1);
        }
        leProximaPalavra(str_num_pass);
        r.num_pass = atoi(str_num_pass);
        free(str_num_pass);
        if(verifica_reservas(r, x) == 0){
            return 0;
        }
        reservas_arr = realloc(reservas_arr, (contador_reservas+1)*sizeof(reservas));
        if(reservas_arr == NULL){
            printf("No memory\n");
            free(r.str_reservas);
            for(i = 0; i < contador_reservas; i++){
                free(reservas_arr[i].str_reservas);
            }
            free(reservas_arr);
            exit(1);
        }
        reservas_arr[contador_reservas] = r;
        passageiros[r.indice_voo] = passageiros[r.indice_voo] - r.num_pass;
        contador_reservas++;
        return 0;
    }
    else{
        bubble_sort_reservas(r.cod_voo, dia, mes, ano);
    }
    return 0;
}

/*Verifica se uma string existe e se e
de um codigo de voo ou uma reserva*/
int ver_string(char* string){
    int i;
    for(i = 0; i < contador_voos; i++){
        if(strcmp(string, IDvoos[i]) == 0 && strcmp(IDvoos[i], "") != 0){
            return 0;
        }
    }
    for(i = 0; i < contador_reservas; i++){
        if(strcmp(string, reservas_arr[i].str_reservas) == 0){
            return i;
        }
    }
    printf("not found\n");
    return -1;
}

/*Elimina um voo do sistema*/
int elimina_voo(char* string){
    int i, j, k, v, cont;
    cont = 0;
    /*Elimina o voo dos arrays que guardam os voos*/
    for(i = 0; i < contador_voos - 1;){
        if(strcmp(IDvoos[i], string) == 0 && compara_apenas_datas(data, Datas[i][1]) == 1){
            for(j = i; j < contador_voos - 1; j++){
                strcpy(IDvoos[j], IDvoos[j+1]);
                strcpy(AeroPart[j], AeroPart[j+1]);
                strcpy(AeroCheg[j], AeroCheg[j+1]);
                passageiros[j] = passageiros[j+1];
                for(k = 0; k < Chegada_Partida; k++){
                    Datas[j][k][0] = Datas[j+1][k][0];
                    Datas[j][k][1] = Datas[j+1][k][1];
                    Datas[j][k][2] = Datas[j+1][k][2];
                    Horas[j][k][0] = Horas[j+1][k][0];
                    Horas[j][k][1] = Horas[j+1][k][1];
                }
                
            }
            contador_voos--;
            v = verifica_aeroporto_criado(AeroPart[i], 1);
            num_voos[v]--;
            cont++;
        }
        else{
            i++;
        }
    }
    if(contador_voos > 0 && strcmp(IDvoos[contador_voos-1], string) == 0 
    && compara_apenas_datas(data, Datas[i][1]) == 1){
        IDvoos[contador_voos-1][0] = '\0';
        AeroPart[contador_voos-1][0] = '\0';
        AeroCheg[contador_voos-1][0] = '\0';
        passageiros[contador_voos-1]= '\0';
        for(k = 0; k < Chegada_Partida; k++){
            Datas[contador_voos-1][k][0] = 0;
            Datas[contador_voos-1][k][1] = 0;
            Datas[contador_voos-1][k][2] = 0;
            Horas[contador_voos-1][k][0] = 0;
            Horas[contador_voos-1][k][1] = 0;
        }
        contador_voos--;
        cont++;
    }
    if(cont == 0){
        printf("not found\n");
        return 0;
    }
    /*Elimina reservas associadas com o voo especificos*/
    for(i = 0; i < contador_reservas - 1;){
        if(strcmp(reservas_arr[i].cod_voo, string) == 0 && 
        compara_apenas_datas(data, Datas[reservas_arr[i].indice_voo][1]) == 1){
            for(j = i; j < contador_reservas - 1; j++){
                reservas_arr[i] = reservas_arr[i+1];        
            }
            contador_reservas--;
        }
        else{
            i++;
        }
    }
    if(strcmp(reservas_arr[contador_reservas-1].cod_voo, string) == 0 
    && compara_apenas_datas(data, Datas[reservas_arr[contador_reservas-1].indice_voo][1]) == 1){
        if(contador_reservas == 1){                
            free(reservas_arr);
        }
        else{
            reservas_arr = realloc(reservas_arr, (contador_reservas-1)*sizeof(reservas));
            reservas_arr = realloc(reservas_arr, (contador_reservas)*sizeof(reservas));
        }
        contador_reservas--;
    }
    return 0;
}

/*Elimina uma reserva do sistema*/
int elimina_reserva(int c){
    int i;
    passageiros[reservas_arr[c].indice_voo] += reservas_arr[c].num_pass;
    if(c < contador_reservas - 1){
        for(i = c; i < contador_reservas - 1; i++){
            reservas_arr[i] = reservas_arr[i+1];
        }
        contador_reservas--;
    }
    else{
        if(contador_reservas == 1){                
            free(reservas_arr);
            reservas_arr = (reservas*)malloc(sizeof(reservas));
        }
        else{
            reservas_arr = realloc(reservas_arr, (contador_reservas-1)*sizeof(reservas));
            reservas_arr = realloc(reservas_arr, (contador_reservas)*sizeof(reservas));
        }
        contador_reservas--;
    }
    return 0;
}

/*Recebe um input e chama uma funcao dependente desse input*/
int elimina_reserva_voos(){
    int c, i;
    char* string;
    string = (char*)malloc(sizeof(int)*sizeof(string));
    if(string == NULL){
        printf("No memory\n");
        for(i = 0; i < contador_reservas; i++){
            free(reservas_arr[i].str_reservas);
        }
        free(reservas_arr);
        exit(1);
    }
    scanf(" %s", string);
    if((c = ver_string(string)) == -1){
        return 0;
    }
    if(strlen(string) < 10){
        elimina_voo(string);
    }
    else{
        elimina_reserva(c);
    }
    free(string);
    return 0;
}

/*Le o caractere que define uma funcao e chama essa funcao*/
int main(){
    char c;
    reservas_arr = (reservas*)malloc(sizeof(reservas));
    while((c=getchar()) != 'q'){
        if(c == 'a'){
            cria_aeroporto();
        }
        else if(c == 'l'){
            lista_aeroportos();
        }
        else if(c == 'v'){
            lista_voos();
        }
        else if(c == 'p'){
            lista_partidas();
        }
        else if(c == 'c'){
            lista_chegadas();
        }
        else if(c == 't'){
            avanca_data();
        }
        else if(c == 'r'){
            cria_reserva();
        }
        else if(c == 'e'){
            elimina_reserva_voos();
        }
    }
    return 0;
}