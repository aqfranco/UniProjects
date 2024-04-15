# include <unistd.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <stdio.h>
# include <string.h>
# include <stdio.h>
# include <ctype.h>

# define READ_BUFFER_SIZE 512
# define FILE_SIZE_STRING_LENGHT 9
# define BUFFER_SIZE 6001
# define BUFFER2_SIZE 50
# define MAX_BIDS 50
# define PASSWORD_LEN 8
# define ID_LEN 6
# define LISTS_MAX_SIZE 20000


int fd, errcode, buffer_size = BUFFER_SIZE;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[BUFFER_SIZE], port[5] = "58061", *ASIP = "localhost";
char buffer_2[BUFFER2_SIZE];
char ID_1[ID_LEN + 1], password_1[PASSWORD_LEN] = "\0", password_1_aux[PASSWORD_LEN] = "\0";
int logged = 0;
char final_answer_aux[BUFFER_SIZE] = "\0";

int check_string(char* word, int len){
    if (strlen(word) != len){
            return 0;
        }
    for (int i= 0; i < len; i++){
        if (!isdigit(word[i])){
            return 0;
        }
    }
    return 1;
}

char* send_udp(char* message){
    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) exit(1);
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    strcpy(password_1_aux, password_1);
    errcode = getaddrinfo(ASIP, port, &hints, &res);
    if (errcode != 0) exit(1);
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr,&addrlen);
    if (n==-1) exit(1);
    freeaddrinfo(res);
    close(fd);
    strcpy(password_1, password_1_aux);
    logged = 1;
    return buffer;
}

void read_tcp_word(char* word){
    size_t n = 1;
    memset(buffer, 0 , strlen(buffer));
    memset(word, 0, strlen(word));
    while (strcmp(buffer, " ") != 0){
        memset(buffer, 0 , strlen(buffer));
        n = read(fd, buffer, 1);
        if (strcmp(buffer, "\n") == 0 || strcmp(buffer, " ") == 0){
            break;
        }
        strcat(word, buffer);
    }
}

int write_in_file(char* fname, int len){
    FILE *file;
    size_t n = 1, bytes_write;
    file = fopen(fname, "w");
    if (file == NULL) {
        return 0;
    }
    memset(buffer, 0 , strlen(buffer));
    while (len > READ_BUFFER_SIZE) {
        memset(buffer, 0 , strlen(buffer));
        n = read(fd, buffer, READ_BUFFER_SIZE);
        bytes_write = fwrite(buffer, 1, n, file);
        len -= bytes_write;
    }
    memset(buffer, 0 , strlen(buffer));
    n = read(fd, buffer, len);
    bytes_write = fwrite(buffer, 1, n, file);
    memset(buffer, 0 , strlen(buffer));
    n = read(fd, buffer, 1);
    if (strcmp(buffer, "\n") != 0){
        return 0;
    }
    freeaddrinfo(res);
    close(fd);
    fclose(file);
    return 1;
}

char* send_tcp(char* message, char* fname, int show){
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exit(1);
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int logged_aux = logged;
    strcpy(password_1_aux, password_1);
    errcode = getaddrinfo(ASIP, port, &hints, &res); 
    if (errcode != 0) exit(1);
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);


    n = write(fd, message, strlen(message));
    if (strcmp(fname, "") != 0){
        FILE *file;
        size_t fileSize;
        size_t bytes_read;
        file = fopen(fname, "r");

        if (file == NULL) {
            return 0;
        }

        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        int len_buffer;

        if (fileSize < READ_BUFFER_SIZE){
            len_buffer = fileSize;
        } else {
            len_buffer = READ_BUFFER_SIZE;
        }
        char buffer_write[len_buffer];
        char file_size[FILE_SIZE_STRING_LENGHT];
        int maxLength = snprintf(NULL, 0, "%zu", fileSize);
        snprintf(file_size, maxLength + 1, "%zu", fileSize);
        strcat(file_size, " ");
        n = write(fd, file_size, strlen(file_size));

        while (fileSize > 0) {
            memset(buffer_write, 0 , strlen(buffer_write));
            bytes_read = fread(buffer_write, 1, READ_BUFFER_SIZE, file);
            n = write(fd, buffer_write, bytes_read);
            fileSize -= bytes_read;
            memset(buffer_write, 0 , strlen(buffer_write));
        }

        n = write(fd, "\n", 1);
        memset(file_size, 0, FILE_SIZE_STRING_LENGHT);
        memset(buffer_write, 0, strlen(buffer_write));
        fclose(file);
    }

    if (n == -1) exit(-1);
    int nes = 4;
    if (show == 1){
        while (nes > 0){
            memset(buffer_2, 0, BUFFER2_SIZE);
            n = read(fd, buffer, nes);
            strcat(buffer_2, buffer);
            memset(buffer, 0, BUFFER_SIZE);
            nes -= n;
        }
        strcpy(password_1, password_1_aux);
        logged = logged_aux;
        return buffer_2;
    }
    n = read(fd, buffer, BUFFER_SIZE);

    if (n == -1) exit(1);

    freeaddrinfo(res);
    close(fd);
    strcpy(password_1, password_1_aux);
    logged = 1;
    return buffer;
}

void login_write(int value){
    if (value == -1){
        printf("incorrect login attempt\n");
    } else if(value == 0) {
        printf("user sucessfuly registered\n");
    } else if (value == 1){
        printf("login sucessful\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int login_user(char* password, char* ID){
    if (ID == NULL || password == NULL){
        return -1;
    }
    char message[21];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10];
    int len;
    if (strlen(password) != PASSWORD_LEN){
        return -1;
    }
    if (!check_string(ID, ID_LEN)){
        return -1;
    }
    for (int i = 0; i < PASSWORD_LEN; i++){
        if (!isalnum(password[i])){
            return -1;
        }
    }
    snprintf(message, 21, "LIN %s %s\n", ID, password);
    strcpy(answer, send_udp(message));
    len = strlen(answer);
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RLI ") != 0){
        return -1;
    }
    strcpy(status, answer + 4);
    if (strcmp(status, "OK\n") == 0){
        strcpy(ID_1, ID);
        strcpy(password_1, password);
        logged = 1;
        return 1;
    } else if (strcmp(status, "NOK\n") == 0){
        return -1;
    } else if (strcmp(status, "REG\n")== 0){
        strcpy(ID_1, ID);
        strcpy(password_1, password);
        logged = 1;
        return 0;
    }
    return 2;
}

void logout_write(int value){
    if (value == -1){
        printf("incorrect logout attempt\n");
    } else if(value == 0) {
        printf("user not logged in\n");
    } else if (value == 1){
        printf("logout successful\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    } else if (value == 3){
        printf("unknown user\n");
    }
}

int logout_user(){
    if (logged == 0){
        return 0;
    }
    char message[21];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10]; 
    snprintf(message, 20, "LOU %s %s", ID_1, password_1);
    strcat(message, "\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RLO ") != 0){
        memset(message, 0, 21);
        return -1;
    }
    strcpy(status, answer + 4);
    if (strcmp(status, "OK\n") == 0){
        logged = 0;
        memset(message, 0, 21);
        return 1;
    } else if (strcmp(status, "NOK\n") == 0){
        memset(message, 0, 21);
        return 0;
    } else if (strcmp(status, "UNR\n")== 0){ 
        memset(message, 0, 21);
        return 3;
    }
    memset(message, 0, 21);
    return 2;

}   

void unregister_write(int value){
    if (value == -1){
        printf("incorrect unregister attempt\n");
    } else if(value == 0) {
        printf("user not logged in\n");
    } else if (value == 1){
        printf("successful unregister\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    } else if (value == 3){
        printf("unknown user\n");
    }
}

int unregister_user(){

    if (logged == 0){
        return 0;
    }
    char message[21];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10]; 
    int len;
    snprintf(message, 20, "UNR %s %s", ID_1, password_1);
    strcat(message,"\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RUR ") != 0){
        return -1;
    }
    strcpy(status, answer + 4);
    if (strcmp(status, "OK\n") == 0){
        logged = 0;
        return 1;
    } else if (strcmp(status, "NOK\n") == 0){
        return 0;
    } else if (strcmp(status, "UNR\n")== 0){
        return 3;
    }
    return 2;
}

int check_list(char* list){
    char list_aux[BUFFER_SIZE];
    int i, acabou = 0, len;
    strcpy(list_aux, list);
    len = strlen(list_aux);
    while (1){
        char auction[20] = "\0", status[10] = "\0";
        for (i = 0; i < len; i++){
            char letter;
            letter = list_aux[i];
            if (letter == ' '){
                break;
            }
            if (letter == '\n'){
                acabou = 1;
                break;
            }
            strncat(auction, &letter, 1);
        }
        memmove(list_aux, list_aux + i + 1, strlen(list_aux + i + 1) + 1);
        len = strlen(list_aux);
        if (!check_string(auction, 3)){
            return 0;
        }
        for (i = 0; i < len; i++){
            char letter;
            letter = list_aux[i];
            if (letter == ' '){
                break;
            }
            if (letter == '\n'){
                acabou = 1;
                break;
            }
            strncat(status, &letter, 1);    
        }
        memmove(list_aux, list_aux + i+1, strlen(list_aux + i+1) + 1);
        len = strlen(list_aux);
        if (!check_string(status, 1)){
            return 0;
        }
        memset(auction, 0, 20);
        memset(status, 0, 10);
        if (acabou == 1){
            break;
        }
    }
    return 1;
}

void str_replace(char *target, const char *needle, const char *replacement){
    char buffer[LISTS_MAX_SIZE] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        tmp = p + needle_len;
    }

    strcpy(target, buffer);
}

void myauctions_write(int value){
    if (value == -1){
        printf("incorrect attempt to myauctions\n");
    } else if(value == 0) {
        printf("user not logged in\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    } else if (value == 3){
        printf("no ongoing auctions\n");
    }
}

int my_auctions(){
    if (logged == 0){
        return 0;
    }
    char message[12];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10]; 
    snprintf(message, 11, "LMA %s", ID_1);
    strcat(message, "\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RMA ") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE], final_part[LISTS_MAX_SIZE], final_answer[LISTS_MAX_SIZE];
    strcpy(new_answer, answer + 4);
    for (int i = 0; i < 3; i++){
        strncat(status, &new_answer[i], 1); 
    }
    if (strcmp(status, "OK ") == 0){               
        strcpy(final_part, new_answer + 3);
        if (!check_list(final_part)){
            return -1;
        }
        str_replace(final_part, " 0 ", " - not active\n");
        str_replace(final_part, " 1 ", " - active\n");
        str_replace(final_part, " 1\n", " - active\n");
        str_replace(final_part, " 0\n", " - not active\n");
        strcpy(final_answer, "List of auctions:\n");
        strcat(final_answer ,final_part);
        printf("%s", final_answer);
        memset(new_answer, 0, BUFFER_SIZE);
        memset(final_answer, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        memset(final_part, 0, LISTS_MAX_SIZE);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        return 3;
    } else if (strcmp(status, "NLG")== 0){
        memset(status, 0, 10);
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, LISTS_MAX_SIZE);
        return 0;
    }
    memset(status, 0, 10);
    return 2;
}

void mybids_write(int value){
    if (value == -1){
        printf("incorrect attempt to mybids\n");
    } else if(value == 0) {
        printf("user not logged in\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    } else if (value == 3){
        printf("no ongoing bids\n");
    }
}

int my_bids(){
    if (logged == 0){
        return 0;
    }
    char message[12]; 
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10]; 
    snprintf(message, 11, "LMB %s", ID_1);
    strcat(message, "\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RMB ") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE], final_part[LISTS_MAX_SIZE], final_answer[LISTS_MAX_SIZE];
    strcpy(new_answer, answer + 4);
    for (int i = 0; i < 3; i++){
        strncat(status, &new_answer[i], 1); 
    }
    if (strcmp(status, "OK ") == 0){
        strcpy(final_part, new_answer + 3);
        if (!check_list(final_part)){
            return -1;
        }
        str_replace(final_part, " 0 ", " - not active\n");
        str_replace(final_part, " 1 ", " - active\n");
        str_replace(final_part, " 1\n", " - active\n");
        str_replace(final_part, " 0\n", " - not active\n");
        strcpy(final_answer, "List of bids:\n");
        strcat(final_answer ,final_part);
        printf("%s", final_answer);
        memset(new_answer, 0, BUFFER_SIZE);
        memset(final_answer, 0, LISTS_MAX_SIZE);
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        return 3;
    } else if (strcmp(status, "NLG")== 0){
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        return 0;
    }
    memset(status, 0, 10);
    return 2;
}

void list_write(int value){
    if (value == -1){
        printf("incorrect attempt to list\n");
    } else if(value == 0) {
        printf("no ongoing bids\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int list(){
    char message[5];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10] ="\0"; 
    strcpy(message, "LST\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RLS ") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE] = "\0", final_part[LISTS_MAX_SIZE]= "\0", last_answer[LISTS_MAX_SIZE]= "\0";
    strcpy(new_answer, answer + 4);
    for (int i = 0; i < 3; i++){
        strncat(status, &new_answer[i], 1); 
    }
    if (strcmp(status, "OK ") == 0){               
        strcpy(final_part, new_answer + 3);
        if (!check_list(final_part)){
            return -1;
        }
        str_replace(final_part, " 0 ", " - not active\n");
        str_replace(final_part, " 1 ", " - active\n");
        str_replace(final_part, " 1\n", " - active\n");
        str_replace(final_part, " 0\n", " - not active\n");
        strcpy(last_answer, "List of bids:\n");
        memset(answer, 0, BUFFER_SIZE);
        memset(first_part, 0, 10);
        memset(status, 0, 10);
        memset(new_answer, 0, BUFFER_SIZE);
        strcat(last_answer,final_part);
        printf("%s", last_answer);
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(last_answer, 0, LISTS_MAX_SIZE);
        memset(message, 0, 5);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, LISTS_MAX_SIZE);
        memset(status, 0, 10);
        return 0; 
    }
    memset(status, 0, 10);
    return 2; 
}

void show_record_write(int value){
    if (value == -1){
        printf("incorrect attempt to show_record\n");
    } else if(value == 0) {
        printf("auction does not exist\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int compare_first_two_characters(){
    char first[1] = "\0", second[1] = "\0";
    strncat(first, &final_answer_aux[0], 1);
    if (strcmp(first, " ") != 0){
        memset(first, 0, 1);
        return 0;
    }
    memmove(final_answer_aux, final_answer_aux + 1, strlen(final_answer_aux + 1) + 1);
    strncat(second, &final_answer_aux[0], 1);
    if (strcmp(second, " ") == 0){
        memset(second, 0, 1);
        return 0;
    }
    memset(first, 0, 1);
    return 1;
}

int compare_first_two_characters_frase(char* frase){
    if (frase[0] != ' '){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    if (frase[0] == ' '){
        return 0;
    }
    return 1;
}

int pass_next_word(char* frase){
    int i = 0;
    for (i;; ++i){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
    }
    return i;
}

int check_start_show_answer(char* frase){
    char ID[ID_LEN + 1] = "";
    for (int i = 0; i < ID_LEN; i++){
        strncat(ID, &frase[i], 1);
    }
    if (!check_string(ID, ID_LEN)){
        return 0;
    }
    memmove(frase, frase + ID_LEN, strlen(frase)+ID_LEN + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    int i = pass_next_word(frase);
    memmove(frase, frase + i, strlen(frase + i) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    i = pass_next_word(frase);
    memmove(frase, frase + i, strlen(frase + i) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    char price[20] = "\0";
    i = 0;
    for (i; i < 6; i++){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
        strncat(price, &frase[i], 1);
    }
    if (atoi(price) < 1){
        return 0;
    }
    memmove(frase, frase + i, strlen(frase + i) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 19, strlen(frase + 19) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    char time[5] = "\0";
    i = 0;
    for (i; i < 5; i++){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
        strncat(time, &frase[i], 1);
    }
    if (atoi(time) < 1){
        return 0;
    }
    memmove(frase, frase + i, strlen(frase + i) + 1);
    char first[1] = "\0";
    strncat(first, &frase[0], 1);
    if (strcmp(first, "\n") == 0){
        memset(first, 0, 1);
        return 1;
    }
    return 2;
}

int check_bids_show(char* frase){
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    char ID[ID_LEN] = "\0";
    for (int i = 0; i < ID_LEN; i++){
        strncat(ID, &frase[i], 1);
    }
    if (!check_string(ID, ID_LEN)){
        return 0;
    }
    memmove(frase, frase + ID_LEN, strlen(frase + ID_LEN) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    char price[20] = "\0";
    int i = 0;
    for (i;; i++){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
        strncat(price, &frase[i], 1);
    }
    if (atoi(price) < 1){
        return 0;
    }
    memmove(frase, frase + i, strlen(frase + i) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    memmove(frase, frase + 19, strlen(frase + 19) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    char time[5] = "\0";
    i = 0;
    for (i; i < 5; i++){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
        strncat(time, &frase[i], 1);
    }
    if (atoi(time) < 1){
        return 0;
    }
    memmove(frase, frase + i, strlen(frase + i) + 1);
    char first[1] = "\0";
    strncat(first, &frase[0], 1);
    if (strcmp(first, "\n") == 0){
        memset(first, 0, 1);
        return 1;
    }
    return 2;
}

int check_closed_auction(char* frase){
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    if (!compare_first_two_characters()){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);  
    memmove(frase, frase + 19, strlen(frase + 19) + 1);
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    char time[20] = "\0";
    int i = 0;
    for (i;; i++){
        if (frase[i] == ' '){
            break;
        }
        if (frase[i] == '\n'){
            break;
        }
        strncat(time, &frase[i], 1);
    }
    if (atoi(time) < 1){
        return 0;
    }
    memmove(frase, frase + i, strlen(frase + i) + 1);
    char first[1] = "\0";
    strncat(first, &frase[0], 1);
    if (strcmp(first, "\n") == 0){
        memset(first, 0, 1);
        return 1;
    }
    return 0;
}

int check_show_record_answer(char* frase){
    int value = check_start_show_answer(frase);
    if (value == 0){
        return 0;
    } else if (value == 1){
        return 1;
    }
    char first[1] = "\0";
    if (!compare_first_two_characters_frase(frase)){
        return 0;
    }
    memmove(frase, frase + 1, strlen(frase + 1) + 1);
    int bids = 0;
    while (bids < MAX_BIDS){
        if (frase[0] == 'B'){
            value = check_bids_show(frase);
        } else {
            break;
        }
        if (value == 0){
            return 0;
        } else if (value == 1){
            return 1;
        } 
        if (!compare_first_two_characters_frase(frase)){
            return 0;
        }
        memmove(frase, frase + 1, strlen(frase + 1) + 1);
        bids++;
    }
    if (bids == MAX_BIDS && final_answer_aux[0] == 'B'){
        return 0;
    }
    if (final_answer_aux[0] == 'E'){
        value = check_closed_auction(frase);
    }
    return value;
}

int show_record(char* AID){
    if (AID == NULL){
        return -1;
    }
    if (!check_string(AID, 3)){
        return -1;
    }
    char message[9];
    char answer[BUFFER_SIZE], first_part[10] = "\0", status[10] = "\0"; 
    snprintf(message, 8, "SRC %s", AID);
    strcat(message, "\n");
    strcpy(answer, send_udp(message));
    for (int i = 0; i < 4; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RRC ") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE] = "\0", final_part[BUFFER_SIZE] = "\0", final_answer[BUFFER_SIZE] = "\0", final_part_aux[BUFFER_SIZE] = "\0";
    strcpy(new_answer, answer + 4);
    for (int i = 0; i < 3; i++){
        strncat(status, &new_answer[i], 1); 
    }
    if (strcmp(status, "OK ") == 0){               
        strcpy(final_part, new_answer + 3);
        strcpy(final_part_aux, final_part);
        if (!check_show_record_answer(final_part_aux)){
            memset(final_part, 0, BUFFER_SIZE);
            return -1;
        }

        strcpy(final_answer, "Auction: ");
        strcat(final_answer ,final_part);
        printf("%s", final_answer);
        memset(new_answer, 0, BUFFER_SIZE);
        memset(final_answer, 0, BUFFER_SIZE);
        memset(final_part, 0, BUFFER_SIZE);
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        strcpy(final_part, new_answer + 3);
        if (strcmp(final_part, "\n") != 0){
            return -1;
        }
        memset(final_part, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 0;
    }
    return 2;
}

int get_next_word(char* word, int max_len){
    int i = 0;
    for (i;i < max_len; ++i){
        if (final_answer_aux[i] == ' '){
            break;
        }
        if (final_answer_aux[i] == '\n'){
            break;
        }
        char letter[2] = {final_answer_aux[i], '\0'};
        strcat(word, letter);
    }
    return i;
}

void open_write(int value){
    if (value == -2){
        printf("need to login first\n");
    } else if(value == -1) {
        printf("incorrect attempt to create auction\n");
    } else if(value == 0) {
        printf("couldn't create auction\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int check_fname(char* asset_fname){
    if (strlen(asset_fname) > 24 || strlen(asset_fname) < 5){
        return 0;
    } 
    char fname[24] = "\0";
    strcpy(fname, asset_fname);
    memmove(fname, fname+ strlen(fname)-4, strlen(fname + strlen(fname)- 4) + 1);
    if (fname[0] != '.'){
        return 0;
    }
    memmove(fname, fname + 1, strlen(fname + 1) + 1);
    for (int i = 0; i < 3; i++){
        if (!isalpha(fname[i])){
            return 0;
        }
    }
    memset(fname, 0, strlen(fname));
    return 1;
}

int open_auction(char* time_active, char* start_value, char* asset_fname, char* name){
    if (time_active == NULL || start_value == NULL || asset_fname == NULL || name == NULL){
        return -1;
    }
    if (logged == 0){
        return -2;
    }
    if (strlen(name) > 10 || strlen(start_value) > 6 || strlen(time_active) > 5){
        return -1;
    }
    if (!check_fname(asset_fname)){
        return -1;
    }
    if (access(asset_fname, F_OK) != 0){
        return -1;
    }
    char message[100] = "\0";
    char answer[BUFFER_SIZE] = "\0", first_part[10] = "\0", status[10] = "\0"; 
    snprintf(message, 99, "OPA %s %s %s %s %s %s", ID_1, password_1, name, start_value, time_active, asset_fname);
    strcat(message, " ");
    strcpy(answer, send_tcp(message, asset_fname, 0));
    for (int i = 0; i < 3; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "ROA") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE], final_part[BUFFER_SIZE], final_answer[BUFFER_SIZE];
    strcpy(final_answer_aux, answer + 3);
    if (!compare_first_two_characters){
        return 0;
    }
    memmove(final_answer_aux, final_answer_aux + 1, strlen(final_answer_aux + 1) + 1);
    int value = get_next_word(status, 3);
    if (value == 0){
        return 0;
    }
    memmove(final_answer_aux, final_answer_aux + value, strlen(final_answer_aux + value) + 1);
    if (strcmp(status, "OK") == 0){
        if (!compare_first_two_characters){
            memset(final_answer_aux, 0, BUFFER_SIZE);
            return 0;
        }
        memmove(final_answer_aux, final_answer_aux + 1, strlen(final_answer_aux + 1) + 1);
        char auction[4] = "\0";
        if (get_next_word(auction, 3) == 0){
            memset(final_answer_aux, 0, BUFFER_SIZE);
            return 0;
        }
        printf("New auction created: %s\n", auction);
        memset(final_answer_aux, 0, 100);
        memset(message, 0, strlen(message));
        memset(time_active, 0, strlen(time_active));
        memset(name, 0, strlen(name));
        memset(asset_fname, 0, strlen(asset_fname));
        memset(start_value, 0, strlen(start_value));
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        memset(final_answer_aux, 0, BUFFER_SIZE);
        return 0;
    } else if (strcmp(status, "NLG") == 0){
        memset(final_answer_aux, 0, BUFFER_SIZE);
        return -2;
    }
    memset(final_answer_aux, 0, BUFFER_SIZE);
    return 2;
}

void close_write(int value){
    if (value == -2){
        printf("need to login first\n");
    } else if(value == -1) {
        printf("incorrect attempt to close auction\n");
    } else if(value == 0) {
        printf("couldn't close auction\n");
    } else if (value == 1){
        printf("auction successfuly closed\n");
    } else if (value == 2){
        printf("invalid user ID or password\n");
    } else if (value == 3){
        printf("auction ID doesn't exist\n");
    } else if (value == 4){
        printf("auction not from this user\n");
    } else if (value == 5){
        printf("auction already closed\n");
    } else if (value == 6){
        printf("NOT WORKED\n");
    }
}

int close_auction(char* AID){
    if (AID == NULL){
        return -1;
    }
    if (logged == 0){
        return -2;
    }
    if (strlen(AID) != 3){
        return -1;
    }
    for (int i = 0; i < 3; i++){
        if (!isdigit(AID[i])){
            return -1;
        }
    }
    char message[100];
    char answer[BUFFER_SIZE] = "\0", first_part[10] = "\0", status[10] = "\0"; 
    snprintf(message, 99, "CLS %s %s %s", ID_1, password_1, AID);
    strcat(message, "\n");
    strcpy(answer, send_tcp(message, "", 0));
    for (int i = 0; i < 3; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RCL") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE], final_part[BUFFER_SIZE], final_answer[BUFFER_SIZE];
    strcpy(final_answer_aux, answer + 3);
    if (!compare_first_two_characters){
        return 0;
    }
    memmove(final_answer_aux, final_answer_aux + 1, strlen(final_answer_aux + 1) + 1);
    int value = get_next_word(status, 3);
    if (value == 0){
        return 0;
    }
    if (strcmp(status, "OK") == 0){
        memmove(final_answer_aux, final_answer_aux + 2, strlen(final_answer_aux + 2) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 2;
    } else if (strcmp(status, "NLG") == 0){
        memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return -2;
    } else if (strcmp(status, "EAU") == 0){
        memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 3;
    } else if (strcmp(status, "EOW") == 0){
        memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 4;
    } else if (strcmp(status, "END") == 0){
        memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
        if (strcmp(final_answer_aux, "\n") != 0){
            return -1;
        }
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return 5;
    }
    return 6;
}

void bid_write(int value){
    if (value == -2){
        printf("need to login first\n");
    } else if(value == -1) {
        printf("incorrect attempt to bid\n");
    } else if(value == 0) {
        printf("couldn't bid auction\n");
    } else if (value == 1){
        printf("auction successfuly bidded\n");
    } else if (value == 2){
        printf("auction not active\n");
    } else if (value == 3){
        printf("larger bid already made for this auction\n");
    } else if (value == 4){
        printf("can't bid to your own auction\n");
    } else if (value == 5){
        printf("NOT WORKED\n");
    }
}

int bid_auction(char* AID, char* value_bid){
    if (AID == NULL || value_bid == NULL){
        return -1;
    }
    if (logged == 0){
        return -2;
    }
    if (!check_string(AID, 3)){
        return -1;
    }

    char message[100];
    char answer[BUFFER_SIZE] = "\0", first_part[10] = "\0", status[10] = "\0"; 
    snprintf(message, 99, "BID %s %s %s %s", ID_1, password_1, AID, value_bid);
    strcat(message, "\n");
    strcpy(answer, send_tcp(message, "", 0));
    for (int i = 0; i < 3; i++){
        strncat(first_part, &answer[i], 1); 
    }
    if (strcmp(first_part, "RBD") != 0){
        return -1;
    }
    char new_answer[BUFFER_SIZE], final_part[BUFFER_SIZE], final_answer[BUFFER_SIZE];
    strcpy(final_answer_aux, answer + 3);
    if (!compare_first_two_characters){
        return 0;
    }
    memmove(final_answer_aux, final_answer_aux + 1, strlen(final_answer_aux + 1) + 1);
    int value = get_next_word(status, 3);
    if (value == 0){
        return 0;
    }

    memmove(final_answer_aux, final_answer_aux + 3, strlen(final_answer_aux + 3) + 1);
    if (strcmp(final_answer_aux, "\n") != 0){
        memset(final_answer_aux, 0, BUFFER_SIZE);
        memset(status, 0, 10);
        return -1;
    }
    memset(final_answer_aux, 0, BUFFER_SIZE);

    if (strcmp(status, "ACC") == 0){
        memset(status, 0, 10);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        memset(status, 0, 10);
        return 2;
    } else if (strcmp(status, "NLG") == 0){
        memset(status, 0, 10);
        return -2;
    } else if (strcmp(status, "REF") == 0){
        memset(status, 0, 10);
        return 3;
    } else if (strcmp(status, "ILG") == 0){
        memset(status, 0, 10);
        return 4;
    }
    memset(status, 0, 10);
    return 5;
}

void show_asset_write(int value){
    if (value == -1){
        printf("incorrect show asset attempt\n");
    } else if(value == 0) {
        printf("problem sending asset\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int show_asset(char* AID){
    if (AID == NULL){
        return -1;
    }
    if (!check_string(AID, 3)){
        return -1;
    }
    char message[100];
    char answer[BUFFER_SIZE] = "\0", status[10] = "\0"; 
    snprintf(message, 99, "SAS %s", AID);
    strcat(message, "\n");
    strcpy(answer, send_tcp(message, "", 1));
    if (strcmp(answer, "RSA ") != 0){
        return -1;
    }
    read_tcp_word(status);
    if (strcmp(status, "OK") == 0){
        char fname[24] = "\0", len[8] = "\0";
        read_tcp_word(fname);
        if (!check_fname(fname)){
            return -1;
        } 
        read_tcp_word(len);

        write_in_file(fname,atoi(len));

        printf("Auction: %s, size: %s\n", fname, len);
        return 1;
    } else if (strcmp(status, "NOK") == 0){
        return 0;
    }
    return 2;
}

void exit_user_write(int value){
    if (value == 0){
        printf("need to logout first\n");
    } else if (value == 1){
        printf("A encerrar aplicação\n");
    } else if (value == 2){
        printf("NOT WORKED\n");
    }
}

int exit_user(){
    if (logged){
        return 0;
    } else {
        return 1;
    }
    return 2;
}

int main(int argc, char *argv[]){
    if (argc == 3){
        if (strcmp(argv[1], "-n") == 0){
            ASIP = (char *)malloc(strlen(argv[2]) + 1);
            strcpy(ASIP, argv[2]);
        } else if (strcmp(argv[1], "-p") == 0){
            strcpy(port, argv[2]);
            if(atoi(port) < 0 || atoi(port) > 65535){
                exit(1);
            }
        }
    }
    else if(argc == 5){
        ASIP = (char *)malloc(strlen(argv[2]) + 1);
        strcpy(ASIP, argv[2]);
        strcpy(port, argv[4]);
        if(atoi(port) < 0 || atoi(port) > 65535){
            exit(1);
        }
    }

    char comando[BUFFER2_SIZE], comando_aux[BUFFER2_SIZE];
    int loop = 1;
    char first_word[BUFFER2_SIZE] = "\0";
    int value;
    while(loop){ 
        fgets(comando, buffer_size, stdin);
        strcpy(comando_aux, comando);
        ssize_t len = strcspn(comando_aux, " \n");
        strncpy(first_word, comando_aux,len);
        if (strcmp(first_word, "login") == 0){
            strtok(comando_aux, " ");
            value = login_user(strtok(NULL, "\n"), strtok(NULL, " "));
            login_write(value);
        } else if (strcmp(first_word, "logout") == 0){
            value = logout_user();
            logout_write(value);
        } else if (strcmp(first_word, "unregister") == 0){
            value = unregister_user();
            unregister_write(value);
        } else if (strcmp(first_word, "myauctions") == 0 || strcmp(first_word, "ma") == 0){
            value = my_auctions();
            myauctions_write(value);
        } else if (strcmp(first_word, "mybids") == 0 || strcmp(first_word, "mb") == 0){
            value = my_bids();
            mybids_write(value);
        } else if (strcmp(first_word, "l") == 0 || strcmp(first_word, "list") == 0){
            value =  list();
            list_write(value);
        } else if (strcmp(first_word, "sr") == 0 || strcmp(first_word, "show_record") == 0){
            strtok(comando_aux, " ");
            value = show_record(strtok(NULL, " \n"));
            show_record_write(value);
        } else if (strcmp(first_word, "open") == 0){
            strtok(comando_aux, " ");
            value = open_auction(strtok(NULL, "\n"), strtok(NULL, " "), strtok(NULL, " "), strtok(NULL, " "));
            open_write(value);
        } else if (strcmp(first_word, "close") == 0){
            strtok(comando_aux, " ");
            value = close_auction(strtok(NULL, " \n"));
            close_write(value);
        } else if (strcmp(first_word, "bid") == 0 || strcmp(first_word, "b") == 0){
            char AID[10], value_bid[10];
            sscanf(comando_aux,"%s %s %s", first_word, AID, value_bid); 
            value = bid_auction(AID, value_bid);
            bid_write(value);
        } else if (strcmp(first_word, "show_asset") == 0 || strcmp(first_word, "sa") == 0){
            char AID[10];
            sscanf(comando_aux,"%s %s", first_word, AID);
            value = show_asset(AID);
            show_asset_write(value);
        } else if (strcmp(first_word, "exit") == 0){
            value = exit_user();
            exit_user_write(value);
            if (value == 1){
                return 0;
            }
        } else {
            printf("Comando inválido\n");
        }
        memset(comando, 0, BUFFER2_SIZE);
        memset(comando_aux, 0, BUFFER2_SIZE);
        memset(first_word, 0, BUFFER2_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
    }
    free(ASIP);


}