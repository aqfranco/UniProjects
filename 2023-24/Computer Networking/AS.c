#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>

#define MAX_UPD_MESSAGE_LEN 21
#define MAX_UDP_REPLY_LEN 6001
#define MAX_TCP_NO_DATA 90
#define MAX_TCP_LEN 10000079
#define DEFAULT_PORT "58011"
#define min(a,b) (((a)<(b))?(a):(b))

int close_auction_automatically(char *AID);
bool verbose = false;
char *PORT = DEFAULT_PORT;
char udp_reply[MAX_UDP_REPLY_LEN] = "";
char tcp_reply[MAX_TCP_NO_DATA] = "";
char tcp_message[MAX_TCP_NO_DATA] = "";
char udp_message[MAX_UPD_MESSAGE_LEN] = "";
char *IP;
char client_port[6];

int login(char *UID, char *pass){
    char path[35], password[9];
    FILE *fp;
    if(strlen(UID) != 6 || strlen(pass) != 8){
        strcpy(udp_reply, "RLI ERR\n");
        if(verbose == true){
            printf("Login Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        return 0;
    }
    for(int i = 0; i < 8; i++){
        if(!isalnum(pass[i])){
            strcpy(udp_reply, "RLI ERR\n");
            if(verbose == true){
                printf("Login Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            return 0;
        }
        if(i < 6){
            if(!isdigit(UID[i])){
                strcpy(udp_reply, "RLI ERR\n");
                if(verbose == true){
                    printf("Login Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
                }
                return 0;
            }
        }
    }
    mkdir("USERS", 0700);
    sprintf(path, "USERS/%s" , UID);
    mkdir(path, 0700);
    sprintf(path, "USERS/%s/HOSTED" , UID);
    mkdir(path, 0700);
    sprintf(path, "USERS/%s/BIDDED" , UID);
    mkdir(path, 0700);
    sprintf(path, "USERS/%s/%s_pass.txt", UID, UID);
    if(access(path, F_OK) == 0){
        fp=fopen(path, "r");
        fscanf(fp, "%s", password);
        fclose(fp);
        if(strcmp(password, pass) == 0){
            strcpy(udp_reply, "RLI OK\n");
            if(verbose == true){
                printf("UID: %s, Login: User logged in, IP: %s, Port: %s\n", UID, IP, client_port);
            }
            sprintf(path, "USERS/%s/%s_login.txt", UID, UID);
            fp=fopen(path, "w");
            fclose(fp);
            return 0;
        }
        strcpy(udp_reply, "RLI NOK\n");
        if(verbose == true){
            printf("UID: %s, Login: Invalid User Credentials, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        return 0;
    }
    else{
        fp=fopen(path, "w");
        fprintf(fp, "%s", pass);
        fclose(fp);
        sprintf(path, "USERS/%s/%s_login.txt", UID, UID);
        fp=fopen(path, "w");
        fclose(fp);
        strcpy(udp_reply, "RLI REG\n");
        if(verbose == true){
            printf("UID: %s, Login: User Registered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        return 0;
    }
    return 0;
}

int logout(char *UID, char *pass){
    char path[35];
    FILE *fp;
    if(strlen(UID) != 6 || strlen(pass) != 8){
        strcpy(udp_reply, "RLO ERR\n");
        if(verbose == true){
            printf("Logout Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        return -1;
    }
    for(int i = 0; i < 8; i++){
        if(!isalnum(pass[i])){
            strcpy(udp_reply, "RLO ERR\n");
            if(verbose == true){
                printf("Logout Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            return -1;
        }
        if(i < 6){
            if(!isdigit(UID[i])){
                strcpy(udp_reply, "RLO ERR\n");
                if(verbose == true){
                    printf("Logout Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
                }
                return -1;
            }
        }
    }
    sprintf(path, "USERS/%s/%s_login.txt" , UID, UID);
    if(remove(path) == 0){
        strcpy(udp_reply, "RLO OK\n");
        if(verbose == true){
            printf("UID: %s, Logout: User successfully logged out, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        return 1;
    }
    else{
        sprintf(path, "USERS/%s/%s_pass.txt" , UID, UID);
        if(access(path, F_OK) == 0){
            strcpy(udp_reply, "RLO NOK\n");
            if(verbose == true){
                printf("UID: %s, Logout: User is not logged in, IP: %s, Port: %s\n", UID, IP, client_port);
            }
            return 2;
        }
        if(verbose == true){
            printf("UID: %s, Logout: User not registered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(udp_reply, "RLO UNR\n");
        return 3;
    }
    return 0;
}

int unregister(char *UID, char *pass){
    char path[35];
    if(strlen(UID) != 6 || strlen(pass) != 8){
        strcpy(udp_reply, "RUR ERR\n");
        if(verbose == true){
            printf("Unregister Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        return 0;
    }
    for(int i = 0; i < 8; i++){
        if(!isalnum(pass[i])){
            strcpy(udp_reply, "RUR ERR\n");
            if(verbose == true){
                printf("Unregister Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            return 0;
        }
        if(i < 6){
            if(!isdigit(UID[i])){
                if(verbose == true){
                    printf("Unregister Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
                }
                strcpy(udp_reply, "RUR ERR\n");
                return 0;
            }
        }
    }
    verbose = false;
    int log_out = logout(UID, pass);
    verbose = true;
    if(log_out == 1){
        sprintf(path, "USERS/%s/%s_pass.txt" , UID, UID);
        remove(path);
        if(verbose == true){
            printf("UID: %s, Unregister: User successfully unregistered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(udp_reply, "RUR OK\n");
        return 0;
    }
    if(log_out == 2){
        if(verbose == true){
            printf("UID: %s, Unregister: User not logged in, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(udp_reply, "RUR NOK\n");
        return 0;
    }
    if(verbose == true){
        printf("UID: %s,  Unregister: User was not registered, IP: %s, Port: %s\n", UID, IP, client_port);
    }
    strcpy(udp_reply, "RUR UNR\n");
    return 0;
}

int myauctions(char *UID){
    long int diff, abs_time, auction_time;
    int status;
    FILE *fp;
    time_t fulltime;
    char path[35], auction[7];
    if(strlen(UID) != 6){
        if(verbose == true){
            printf("My Auctions Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        strcpy(udp_reply, "RMA ERR\n");
        return 0;
    }
    for(int i = 0; i < 6; i++){
        if(!isdigit(UID[i])){
            if(verbose == true){
                printf("My Auctions Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            strcpy(udp_reply, "RMA ERR\n");
            return 0;
        }
    }
    strcpy(udp_reply, "RMA NOK");
    bool auctions = false;
    int user_auctions = 0;
    for(int i = 1; i < 1000; i++){
        sprintf(path, "AUCTIONS/%03d/START_%03d.txt" , i, i);
        if(access(path, F_OK) != 0){
            break;
        }
        fp = fopen(path, "r");
        fscanf(fp, "%*s %*s %*s %*s %ld %*s %*s %ld", &auction_time, &abs_time);
        fclose(fp);
        time(&fulltime);
        diff = fulltime - abs_time;
        sprintf(auction, "%03d", i);
        if(diff > auction_time){
            close_auction_automatically(auction);
        }
        sprintf(path, "USERS/%s/HOSTED/%03d.txt" , UID, i);
        if(access(path, F_OK) == 0){
            if(auctions == false){
                auctions = true;
                strcpy(udp_reply, "RMA OK");
            }
            user_auctions++;
            sprintf(path, "AUCTIONS/%03d/END_%03d.txt" , i, i);
            if(access(path, F_OK) != 0){
                status = 1;
            }
            else{
                status = 0;
            }
            sprintf(auction, " %03d %d", i, status);
            strcat(udp_reply, auction);
        }
    }
    if(verbose == true){
        switch(user_auctions){
            case 0:
                printf("UID: %s, My Auctions: User does not have auctions, IP: %s, Port: %s\n", UID, IP, client_port);
                break;
            case 1:
                printf("UID: %s, My Auctions: User has 1 auction, IP: %s, Port: %s\n", UID, IP, client_port);
                break;
            default:
                printf("UID: %s, My Auctions: User has %d auctions, IP: %s, Port: %s\n", UID, user_auctions, IP, client_port);
                break;
        }
    }
    strcat(udp_reply, "\n");
    return 0;
}

int mybids(char *UID){
    char path[35], auction[7];
    long int diff, abs_time, auction_time;
    FILE *fp;
    time_t fulltime;
    int status;
    if(strlen(UID) != 6){
        strcpy(udp_reply, "RMB ERR\n");
        if(verbose == true){
            printf("My Bids Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        return 0;
    }
    for(int i = 0; i < 6; i++){
        if(!isdigit(UID[i])){
            strcpy(udp_reply, "RMB ERR\n");
            if(verbose == true){
                printf("My Bids Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            return 0;
        }
    }
    strcpy(udp_reply, "RMB NOK");
    bool auctions = false;
    int user_bids = 0;
    for(int i = 1; i < 1000; i++){
        sprintf(path, "AUCTIONS/%03d/START_%03d.txt" , i, i);
        if(access(path, F_OK) != 0){
            break;
        }
        fp = fopen(path, "r");
        fscanf(fp, "%*s %*s %*s %*s %ld %*s %*s %ld", &auction_time, &abs_time);
        fclose(fp);
        time(&fulltime);
        diff = fulltime - abs_time;
        sprintf(auction, "%03d", i);
        if(diff > auction_time){
            close_auction_automatically(auction);
        }
        sprintf(path, "USERS/%s/BIDDED/%03d.txt" , UID, i);
        if(access(path, F_OK) == 0){
            if(auctions == false){
                auctions = true;
                strcpy(udp_reply, "RMB OK");
            }
            user_bids++;
            sprintf(path, "AUCTIONS/%03d/END_%03d.txt" , i, i);
            if(access(path, F_OK) != 0){
                status = 1;
            }
            else{
                status = 0;
            }
            sprintf(auction, " %03d %d", i, status);
            strcat(udp_reply, auction);
        }
    }
    if(verbose == true){
        switch (user_bids){
            case 0:
                printf("UID: %s, My Bids: User does not have bids, IP: %s, Port: %s\n", UID, IP, client_port);
                break;
            case 1:
                printf("UID: %s, My Bids: User has 1 bid, IP: %s, Port: %s\n", UID, IP, client_port);
                break;
            default:
                printf("UID: %s, My Bids: User has %d bids, IP: %s, Port: %s\n", UID, user_bids, IP, client_port);
                break;
        }
    }
    strcat(udp_reply, "\n");
    return 0;
}

int show_record(char *AID){
    char path[45], auction[95], UID[7], name[11], asset[25], value[7], date[11], hour[9], buffer[40], bid_time[6];
    long int diff, abs_time, auction_time;
    FILE *fp;
    time_t fulltime;
    int len, n_entries, n_bids = 0;
    struct dirent **entry;
    if(strlen(AID) != 3){
        strcpy(udp_reply, "SRC ERR\n");
        if(verbose == true){
            printf("Show Record Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
        }
        return 0;
    }
    for(int i = 0; i < 3; i++){
        if(!isdigit(AID[i])){
            strcpy(udp_reply, "SRC ERR\n");
            if(verbose == true){
                printf("Show Record Invalid Parameters, IP: %s, Port: %s\n", IP, client_port);
            }
            return 0;
        }
    }
    sprintf(path, "AUCTIONS/%s/START_%s.txt" , AID, AID);
    if(access(path, F_OK) != 0){
        strcpy(udp_reply, "RRC NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%s %s %s %s %ld %s %s %ld", UID, name, asset, value, &auction_time, date, hour, &abs_time);
    sprintf(auction, "%s %s %s %s %s %s %ld", UID, name, asset, value, date, hour, auction_time);
    fclose(fp);
    strcat(udp_reply, "RRC OK ");
    time(&fulltime);
    diff = fulltime - abs_time;
    if(diff > auction_time){
        close_auction_automatically(AID);
    }
    strcat(udp_reply, auction);
    sprintf(path, "AUCTIONS/%s/BIDS" , AID);
    n_entries = scandir(path, &entry, 0, alphasort);
    if(n_entries>0){
        while(n_entries--){
            len=strlen(entry[n_entries]->d_name);
            if(len == 10){
                sprintf(path, "AUCTIONS/%s/BIDS/%s", AID, entry[n_entries]->d_name);
                fp = fopen(path, "r");
                fscanf(fp, "%s %s %s %s %s", UID, value, date, hour, bid_time);
                sprintf(buffer, "%s %s %s %s %s", UID, value, date, hour, bid_time);
                strcat(udp_reply, " B ");
                strcat(udp_reply, buffer);
                fclose(fp);
                n_bids++;
            }
            if(n_bids == 50){
                break;
            }
        }
    }
    bool ended = false;
    sprintf(path, "AUCTIONS/%s/END_%s.txt" , AID, AID);
    if(access(path, F_OK) == 0){
        fp = fopen(path, "r");
        fscanf(fp, "%s %s %ld", date, hour, &diff);
        fclose(fp);
        sprintf(buffer, "%s %s %ld", date, hour, diff);
        strcat(udp_reply, " E ");
        strcat(udp_reply, buffer);
        ended = true;
    }
    if(verbose == true){
        switch (n_bids){
            case 0:
                printf("AID: %s, Show Record: Auction has no bids, %s, IP: %s, Port: %s\n", AID, ended ? "Closed" : "Active" ,IP, client_port);
                break;
            case 1:
                printf("AID: %s, Show Record: Auction has 1 bid, %s, IP: %s, Port: %s\n", AID, ended ? "Closed" : "Active", IP, client_port);
                break;
            default:
                printf("AID: %s, Show Record: Auction has %d bids, %s, IP: %s, Port: %s\n", AID, n_bids, ended ? "Closed" : "Active", IP, client_port);
                break;
        }
    }
    strcat(udp_reply, "\n");
    return 0;
}

int list(){
    char path[35], auction[7];
    long int diff, abs_time, auction_time;
    int status;
    FILE *fp;
    time_t fulltime;
    strcpy(udp_reply, "RLS NOK");
    int auctions;
    for(int i = 1; i < 1000; i++){
        sprintf(path, "AUCTIONS/%03d/START_%03d.txt" , i, i);
        if(access(path, F_OK) != 0){
            auctions = i - 1;
            break;
        }
        if(i == 1){
            strcpy(udp_reply, "RLS OK");
        }
        fp = fopen(path, "r");
        fscanf(fp, "%*s %*s %*s %*s %ld %*s %*s %ld", &auction_time, &abs_time);
        fclose(fp);
        time(&fulltime);
        diff = fulltime - abs_time;
        sprintf(auction, "%03d", i);
        if(diff > auction_time){
            close_auction_automatically(auction);
        }
        sprintf(path, "AUCTIONS/%03d/END_%03d.txt" , i, i);
        if(access(path, F_OK) != 0){
            status = 1;
        }
        else{
            status = 0;
        }
        sprintf(auction, " %03d %d", i, status);
        strcat(udp_reply, auction);
    }
    if(verbose == true){
        switch(auctions){
            case 0:
                printf("List: There are no auctions, IP: %s, Port: %s\n" ,IP, client_port);
                break;
            case 1:
                printf("List: There is 1 auction, IP: %s, Port: %s\n" ,IP, client_port);
                break;
            default:
                printf("List: There are %d auctions, IP: %s, Port: %s\n", auctions, IP, client_port);
                break;
        }
    }
    strcat(udp_reply, "\n");
    return 0;
}

int read_message(char *message){
    char *UID, *params, *command, *pass;
    int length = strlen(message);
    params = strtok(message, " ");
    command = params;
    if(length == 19 || length == 10 || length == 7){
        params = strtok(NULL, " ");
        UID = params;
    }
    if(length == 19){
        params = strtok(NULL, " ");
        pass = params;
        if(strcmp(command, "LIN") == 0){
            return login(UID, pass);
        }
        if(strcmp(command, "LOU") == 0){
            return logout(UID, pass);
        }
        if(strcmp(command, "UNR") == 0){
            return unregister(UID, pass);
        }
    }
    if(length == 10){
        if(strcmp(command, "LMA") == 0){
            return myauctions(UID);
        }    
        if(strcmp(command, "LMB") == 0){
            return mybids(UID);
        }
    }
    if(length == 7 && strcmp(command, "SRC") == 0){
        return show_record(UID);
    }
    if(length == 3 && strcmp(command, "LST") == 0){
        return list();
    }
    strcpy(udp_reply, "ERR\n");
    if(verbose == true){
        printf("Unexpected command, IP: %s, Port: %s\n", IP, client_port);
    }
    return 0;
}

int bid_item(char *UID, char *pass, char *AID, char *value){
    char path[40], old_value[7], time_str[20], buffer[9];
    FILE *fp;
    int n_entries, auction_value;
    long int diff, abs_time, auction_time;
    struct dirent **entry;
    time_t fulltime;
    struct tm *current_time;
    sprintf(path, "USERS/%s/%s_pass.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Bid: User Not Registered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RBD NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%s", buffer);
    fclose(fp);
    if(strcmp(buffer, pass) != 0){
        if(verbose == true){
            printf("UID: %s, Bid: Invalid User Credentials, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RBD NOK\n");
        return 0;
    }
    sprintf(path, "USERS/%s/%s_login.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Bid: User not logged in, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RBD NGL\n");
        return 0;
    }
    sprintf(path, "USERS/%s/HOSTED/%s.txt", UID, AID);
    if(access(path, F_OK) == 0){
        if(verbose == true){
            printf("UID: %s, Bid: User cannot bid on own auction, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RBD ILG\n");
        return 0;
    }
    sprintf(path, "AUCTIONS/%s/END_%s.txt", AID, AID);
    if(access(path, F_OK) == 0){
        if(verbose == true){
            printf("UID: %s, Bid: Auction %s not active, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }  
        strcpy(tcp_reply, "RBD NOK\n");
        return 0;
    }
    sprintf(path, "AUCTIONS/%s/START_%s.txt", AID, AID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Bid: Auction %s does not exist, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }  
        strcpy(tcp_reply, "RBD NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%*s %*s %*s %d %ld %*s %*s %ld", &auction_value, &auction_time, &abs_time);
    fclose(fp);
    time(&fulltime);
    diff = fulltime - abs_time;
    if(diff > auction_time){
        if(verbose == true){
            printf("UID: %s, Bid: Auction %s not active, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }  
        close_auction_automatically(AID);
        strcpy(tcp_reply, "RBD NOK\n"); 
        return 0;
    }
    if(auction_value > atoi(value)){
        strcpy(tcp_reply, "RBD REF\n");
        if(verbose == true){
            printf("UID: %s, Bid: Bid Value lower than Auction Value, IP: %s, Port: %s\n", UID, IP, client_port);
        }  
        return 0;
    }
    sprintf(path, "AUCTIONS/%s/BIDS" , AID);
    n_entries = scandir(path, &entry, 0, alphasort);    
    if(n_entries > 2){
        int len = strlen(entry[n_entries-1]->d_name);
        if(len == 10){
            sprintf(path, "AUCTIONS/%s/BIDS/%s" , AID, entry[n_entries-1]->d_name);
            fp = fopen(path, "r");
            fscanf(fp, "%*s %s", old_value);
            fclose(fp);
            if(atoi(old_value) >= atoi(value)){
                if(verbose == true){
                    printf("UID: %s, Bid: Bid Value lower than Auction Value, IP: %s, Port: %s\n", UID, IP, client_port);
                }  
                strcpy(tcp_reply, "RBD REF\n");
                return 0;
            }            
        }
    }
    sprintf(path, "AUCTIONS/%s/BIDS" , AID);
    mkdir(path, 0700);
    sprintf(path, "AUCTIONS/%s/BIDS/%06d.txt" , AID, atoi(value));
    current_time = gmtime(&fulltime);
    sprintf(time_str,"%4d-%02d-%02d %02d:%02d:%02d",
    current_time->tm_year+1900, current_time->tm_mon+1, current_time->tm_mday,
    current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
    fp = fopen(path, "w");
    fprintf(fp, "%s %s %s %ld" , UID, value, time_str, diff);
    fclose(fp);
    sprintf(path, "USERS/%s/BIDDED/%s.txt" , UID,  AID);
    fp = fopen(path, "w");
    fclose(fp);
    strcpy(tcp_reply, "RBD ACC\n");
    if(verbose == true){
        printf("UID: %s, Bid: Bid accepted on Auction %s, IP: %s, Port: %s\n", UID, AID, IP, client_port);
    }  
    return 0;
}

int show_asset(char *AID, int newfd){
    char path[50], asset_fname[25], buffer[256];
    int len, n, copy, read;
    long int auction_time, diff, abs_time;
    FILE *fp;
    time_t fulltime;
    struct dirent **entry;
    sprintf(path, "AUCTIONS/%s/START_%s.txt", AID, AID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("Show Asset: Auction %s does not exist, IP: %s, Port: %s\n", AID, IP, client_port);
        }
        strcpy(tcp_reply, "RSA NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%*s %*s %s %*s %ld %*s %*s %ld", asset_fname, &auction_time, &abs_time);
    fclose(fp);
    time(&fulltime);
    diff = fulltime - abs_time;
    if(diff > auction_time){
        close_auction_automatically(AID);
    }
    sprintf(path, "AUCTIONS/%s/ASSET/%s", AID, asset_fname);
    fp = fopen(path, "rb");
    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    int file_length = len;
    fseek(fp,0,SEEK_SET);
    asset_fname[strlen(asset_fname)]=0;
    sprintf(buffer, "RSA OK %s %d ", asset_fname, len);
    buffer[strlen(buffer)] = 0;
    write(newfd, buffer, strlen(buffer));
    memset(buffer, 0, strlen(buffer));
    while(len > 0){
        copy = min(len, 256);
        read = fread(buffer, 1, copy, fp);
        n = write(newfd, buffer, read);
        if(n==-1){
            perror("ERROR");
            exit(1);
        }
        len -= n;
        memset(buffer, 0, n);
    }
    fclose(fp);
    strcpy(buffer, "\n");
    buffer[strlen(buffer)] = 0;
    write(newfd, buffer, strlen(buffer));
    close(newfd);
    if(verbose == true){
        printf("Show Asset: Auction %s, Filename: %s, Size: %d bytes, IP: %s, Port: %s\n", AID, asset_fname, file_length, IP, client_port);
    }
    return 1;
}

int close_auction_automatically(char *AID){
    char path[40], time_str[20], buffer[30];
    FILE *fp;
    time_t fulltime;
    struct tm *current_time;
    long int abs_time, auction_time;
    int diff;
    sprintf(path, "AUCTIONS/%s/START_%s.txt", AID, AID);
    fp = fopen(path, "r");
    fscanf(fp, "%*s %*s %*s %*s %ld %*s %*s %ld", &auction_time, &abs_time);
    fclose(fp);
    time(&fulltime);
    current_time = gmtime(&fulltime);
    diff = (int)(fulltime - abs_time);
    sprintf(time_str,"%4d-%02d-%02d %02d:%02d:%02d",
    current_time->tm_year+1900, current_time->tm_mon+1, current_time->tm_mday,
    current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
    if(diff > auction_time){
        diff = auction_time;
    }
    sprintf(buffer, "%s %d", time_str, diff);
    sprintf(path, "AUCTIONS/%s/END_%s.txt", AID, AID);
    fp = fopen(path, "w");
    fwrite(buffer, 1, strlen(buffer), fp); 
    fclose(fp);
    return 0;
}

int close_auction(char *UID, char *pass, char *AID){
    char path[40], time_str[20], buffer[30];
    FILE *fp;
    sprintf(path, "USERS/%s/%s_pass.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Close: User Not Registered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%s", buffer);
    fclose(fp);
    if(strcmp(buffer, pass) != 0){
        if(verbose == true){
            printf("UID: %s, Close: Invalid User Credentials, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL NOK\n");
        return 0;
    }
    sprintf(path, "USERS/%s/%s_login.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Close: User not logged in, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL NLG\n");
        return 0;
    }
    sprintf(path, "USERS/%s/HOSTED/%s.txt", UID, AID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Close: User did not create auction %s, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL EOW\n");
        return 0;
    }
    sprintf(path, "AUCTIONS/%s/START_%s.txt", AID, AID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Close: Auction %s does not exist, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL EAU\n");
        return 0;
    }
    sprintf(path, "AUCTIONS/%s/END_%s.txt", AID, AID);
    if(access(path, F_OK) == 0){
        if(verbose == true){
            printf("UID: %s, Close: Auction %s already closed, IP: %s, Port: %s\n", UID, AID, IP, client_port);
        }
        strcpy(tcp_reply, "RCL END\n");
        return 0;
    }
    close_auction_automatically(AID);
    strcpy(tcp_reply, "RCL OK\n");
    if(verbose == true){
        printf("UID: %s, Close: Auction %s closed succesfully, IP: %s, Port: %s\n", UID, AID, IP, client_port);
    }
    return 0;    
}

int open_auction(char *UID, char *pass, char *name, char *value, char *timer, char *fname, char *fdata, int newfd, struct pollfd fds, int AID){
    char buffer[256], path[55];
    FILE *fp;
    int len, poll_time, n, copy;
    time_t fulltime;
    struct tm *current_time;
    char time_str[20];
    len = atoi(fdata);
    n = 1;
    sprintf(path, "USERS/%s/%s_pass.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Open: User Not Registered, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "ROA NOK\n");
        return 0;
    }
    fp = fopen(path, "r");
    fscanf(fp, "%s", buffer);
    fclose(fp);
    if(strcmp(buffer, pass) != 0){
        if(verbose == true){
            printf("UID: %s, Open: Invalid User Credentials, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "ROA NOK\n");
        return 0;
    }
    sprintf(path, "USERS/%s/%s_login.txt", UID, UID);
    if(access(path, F_OK) != 0){
        if(verbose == true){
            printf("UID: %s, Open: User not logged in, IP: %s, Port: %s\n", UID, IP, client_port);
        }
        strcpy(tcp_reply, "ROA NLG\n");
        return 0;
    }
    mkdir("AUCTIONS", 0700);
    sprintf(path, "AUCTIONS/%03d" , AID);
    mkdir(path, 0700);
    sprintf(path, "AUCTIONS/%03d/ASSET" , AID);
    mkdir(path, 0700);
    sprintf(path, "AUCTIONS/%03d/ASSET/%s" , AID, fname);
    fp = fopen(path, "wb");
    while(len > 0 && n > 0){
        poll_time = poll(&fds, 1, 50);
        switch(poll_time){
            case -1:
                perror("Error");
                break;
            case 0:
                n = 0;
                break;
            default:
                copy = min(len, 256);
                n = read(newfd,buffer,copy);
                if(n==-1){
                    fclose(fp);
                    perror("ERROR");
                    exit(1);
                }
                fwrite(buffer, 1, n, fp);
                len -= n;
                memset(buffer, 0, n);
                break;
        }
    }
    fclose(fp);
    n = 1;
    int bytes_after_file = 0;
    int bytes_read = 0;
    while(n > 0){
        poll_time = poll(&fds, 1, 50);
        switch(poll_time){
            case -1:
                perror("Error");
                break;
            case 0:
                n = 0;
                break;
            default:
                n = read(newfd,buffer,1);
                if(n==-1){
                    fclose(fp);
                    perror("ERROR");
                    exit(1);
                }
                if(bytes_read == 1){
                    remove(path);
                    sprintf(path, "AUCTIONS/%03d/ASSET" , AID);
                    remove(path);
                    sprintf(path, "AUCTIONS/%03d" , AID);
                    remove(path);
                    if(verbose == true){
                        printf("UID: %s, Unexpected message, IP: %s, Port: %s\n", UID, IP, client_port);
                    }
                    strcpy(tcp_reply, "ERR\n");
                    return 0;  
                }
                len -= n;
                if(buffer[0] == '\n' && bytes_read == 0){
                    bytes_after_file++;
                }
                bytes_read++;
                memset(buffer, 0, n);
                break;
        }
    }
    if(len > 0 || bytes_after_file != 1){
        remove(path);
        sprintf(path, "AUCTIONS/%03d/ASSET" , AID);
        remove(path);
        sprintf(path, "AUCTIONS/%03d" , AID);
        remove(path);
        strcpy(tcp_reply, "ROA NOK\n");
        return 0;  
    }
    sprintf(buffer, "ROA OK %03d\n", AID);
    strcpy(tcp_reply, buffer);
    memset(buffer, 0, strlen(buffer));
    sprintf(path, "USERS/%s/HOSTED/%03d.txt" , UID,  AID);
    fp = fopen(path, "w");
    fclose(fp);
    sprintf(path, "AUCTIONS/%03d/BIDS" , AID);
    mkdir(path, 0700);
    time(&fulltime);
    current_time = gmtime(&fulltime);
    sprintf(time_str,"%4d-%02d-%02d %02d:%02d:%02d",
    current_time->tm_year+1900, current_time->tm_mon+1, current_time->tm_mday,
    current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
    sprintf(path, "AUCTIONS/%03d/START_%03d.txt" , AID, AID);
    fp = fopen(path, "w");
    sprintf(buffer, "%s %s %s %s %s %s %ld", UID, name, fname, value, timer, time_str, fulltime);
    fwrite(buffer, 1, strlen(buffer), fp);
    fclose(fp);
    if(verbose == true){
        printf("UID: %s, Open: Auction %03d open succesfully, IP: %s, Port: %s\n", UID, AID, IP, client_port);
    }
    return 0;
}

int read_message_tcp(int *spaces, struct pollfd fds, int newfd){
    int time, length = 0;
    char command[4] = "";
    char name[11] = "", UID[7] = "", pass[9] = "", value[7] = "", timer[6] = "", fname[25] = "", fdata[9] = "", AID[4] = "";
    char return_command[4], *ptr, buffer[256], path[45];
    int n = 1;
    int space = 0, copy, new_auction = 0;
    FILE *fp;
    ptr = &tcp_message[0];
    while(n > 0 || space < 8){
        time = poll(&fds, 1, 50);
        switch(time){
            case -1:
                perror("Error");
                break;
            case 0:
                n = 0;
                if(strcmp(command, "CLS") == 0 && length == 24){
                    return close_auction(UID, pass, AID);
                }
                if(strcmp(command, "BID") == 0 && (length - strlen(value)) >= 25){
                    return bid_item(UID, pass, AID, value);
                }
                if(strcmp(command, "SAS") == 0 && length == 8){
                    return show_asset(AID, newfd);
                }
                break;
            default:
                n = read(newfd,ptr,1);
                if(n==-1){
                    perror("ERROR");
                    exit(1);
                }
                if(tcp_message[length] == ' '){
                    if(space == 0){
                        for(int i = 0; i < length; i++){
                            command[i] = tcp_message[i];
                        }
                        if(strcmp(command, "SAS") != 0 && strcmp(command, "OPA") != 0 && strcmp(command, "CLS") != 0 && strcmp(command, "BID") != 0){
                            strcpy(tcp_reply, "ERR\n");
                            return 0;
                        }
                        if(strcmp(command, "SAS") == 0){
                            strcpy(return_command, "RSA");
                        }
                        if(strcmp(command, "BID") == 0){
                            strcpy(return_command, "RBD");
                        }
                        if(strcmp(command, "OPA") == 0){
                            strcpy(return_command, "ROA");
                            for(int i = 1; i < 1000; i++){
                                sprintf(path, "AUCTIONS/%03d/ASSET", i);
                                if(access(path, F_OK) != 0){
                                    new_auction = i;
                                    break;
                                }
                            }
                            if(new_auction == 0){
                                strcpy(tcp_reply, "ROA NOK\n");
                                return 0;
                            }
                        }
                        if(strcmp(command, "CLS") == 0){
                            strcpy(return_command, "RCL");
                        }
                    }
                    for(int i = spaces[space-1] + 1; i < length; i++){
                        if(space == 1){
                            if(length - (spaces[space-1] + 1) != 6){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            if(!isdigit(tcp_message[i])){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            UID[i - (spaces[space-1] + 1)] = tcp_message[i];
                        }
                        if(space == 2){
                            if(length - (spaces[space-1] + 1) != 8){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }          
                            if(!isalnum(tcp_message[i])){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            pass[(i - (spaces[space-1] + 1))] = tcp_message[i];
                        }
                        if(space == 3){
                            if(strcmp(command, "BID") == 0){
                                if(length - (spaces[space-1] + 1) != 3){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!isdigit(tcp_message[i])){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                AID[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                            if(strcmp(command, "OPA") == 0){
                                if(!(length - (spaces[space-1] + 1) >= 1 && length - (spaces[space-1] + 1) <= 10)){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!isalnum(tcp_message[i]) && tcp_message[i] != '-' && tcp_message[i] != '_'){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                name[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                        }
                        if(strcmp(command, "OPA") == 0 && space >= 4 && space < 8){
                            if(space == 4){
                                if(!(length - (spaces[space-1] + 1) >= 1 && length - (spaces[space-1] + 1) <= 6)){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!isdigit(tcp_message[i])){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                value[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                            if(space == 5){
                                if(!(length - (spaces[space-1] + 1) >= 1 && length - (spaces[space-1] + 1) <= 5)){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!isdigit(tcp_message[i])){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                timer[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                            if(space == 6){
                                if(!(length - (spaces[space-1] + 1) >= 4 && length - (spaces[space-1] + 1) <= 24)){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if((!(isalpha(tcp_message[length - (i - (spaces[space-1] + 1))]))) && length - (i - (spaces[space-1] + 1)) < 3){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(tcp_message[length - (i - (spaces[space-1] + 1))] != '.' && length - (i - (spaces[space-1] + 1)) == 3){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!(isalnum(tcp_message[i])) && tcp_message[i] != '-' && tcp_message[i] != '_' && tcp_message[i] != '.'){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                fname[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                            if(space == 7){
                                if(!(length - (spaces[space-1] + 1) >= 1 && length - (spaces[space-1] + 1) <= 8) || (i - (spaces[space-1] + 1) == 0 && length - (spaces[space-1] + 1) == 8) && tcp_message[i] - '0' != 1){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                if(!isdigit(tcp_message[i])){
                                    strcpy(tcp_reply, return_command);
                                    strcat(tcp_reply, " ERR\n");
                                    return 0;
                                }
                                fdata[(i - (spaces[space-1] + 1))] = tcp_message[i];
                            }
                        }
                    }
                    spaces[space] = length;
                    space += 1;
                    if(space == 8 && strcmp(command, "OPA") == 0){
                        return open_auction(UID, pass, name, value, timer, fname, fdata, newfd, fds, new_auction);
                    }
                }
                if(tcp_message[length]=='\n'){
                    for(int i = spaces[space-1] + 1; i < length; i++){
                        if((strcmp(command, "SAS") == 0 && space == 1) || (strcmp(command, "CLS") == 0 && space == 3)){
                            if(length - (spaces[space-1] + 1) != 3){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            if(!isdigit(tcp_message[i])){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            AID[(i - (spaces[space-1] + 1))] = tcp_message[i];
                        }
                        if(strcmp(command, "BID") == 0 && space == 4){
                            if(length - (spaces[space-1] + 1) < 1 || length - (spaces[space-1] + 1) > 6){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            if(!isdigit(tcp_message[i])){
                                strcpy(tcp_reply, return_command);
                                strcat(tcp_reply, " ERR\n");
                                return 0;
                            }
                            value[(i - (spaces[space-1] + 1))] = tcp_message[i];
                        }                        
                    }
                }
                ptr += n;
                length += n;
                break;
        }
    }
    strcpy(tcp_reply, "ERR\n");
    if(verbose == true){
        printf("Unexpected command, IP: %s, Port: %s\n", IP, client_port);
    }
    return 0;
}

int main(int argc, char *argv[]){
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-v") == 0){
            verbose = true;
            continue;
        }
        if (strcmp(argv[i], "-p") == 0){
            PORT = argv[i+1];
            i++;
            if(atoi(PORT) < 0 || atoi(PORT) > 65535){
                exit(1);
            }
        }
    }

    char in_str[128];

    fd_set inputs, testfds;
    struct timeval timeout;

    int i,out_fds,n,errcode, ret, len, copy;
    ssize_t nw;
    char *ptr;
    pid_t thread;

    // socket variables
    struct addrinfo hints, tcp, *res, *res_tcp;
    struct sockaddr_in udp_useraddr, tcp_useraddr;
    struct pollfd fds;
    socklen_t addrlen;
    int ufd, tfd, newfd;

    char host[NI_MAXHOST], service[NI_MAXSERV];

    // TCP SERVER SECTION
    memset(&tcp,0,sizeof tcp);
    tcp.ai_family=AF_INET; //IPv4
    tcp.ai_socktype=SOCK_STREAM; //TCP socket
    tcp.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL,PORT,&tcp,&res_tcp);
    if((errcode)!=0){
        exit(1);
    }

    tfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tfd==-1){
        exit(1);
    }

    n=bind(tfd,res_tcp->ai_addr,res_tcp->ai_addrlen);
    if(n==-1){
        printf("Bind error TCP server\n");
        exit(1);
    }

    if(listen(tfd, 8)==-1){
        exit(1);
    }
    
    if(res_tcp!=NULL){
        freeaddrinfo(res_tcp);
    }
    
    // UDP SERVER SECTION
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

    if((errcode=getaddrinfo(NULL,PORT,&hints,&res))!=0){
        return -1;
    }

    ufd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(ufd==-1){
        return -1;
    }
    n=bind(ufd,res->ai_addr,res->ai_addrlen);
    if(n==-1){
        printf("Bind error UDP server\n");
        exit(1);
    }

    if(res!=NULL)
        freeaddrinfo(res);

    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0,&inputs); // Set standard input channel on
    FD_SET(tfd,&inputs);
    FD_SET(ufd,&inputs); // Set UDP channel on
    int clients = 0;
    //printf("Size of fd_set: %ld\n",sizeof(fd_set));
    //printf("Value of FD_SETSIZE: %d\n",FD_SETSIZE);

    while(1){
        testfds=inputs;
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=1;
        
        out_fds=select(FD_SETSIZE,&testfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) &timeout);
        //printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug
        switch(out_fds){
            case 0:
                break;
            case -1:
                perror("select");
                exit(1);
            default:
                if(FD_ISSET(ufd,&testfds)){
                    addrlen = sizeof(udp_useraddr);
                    ret=recvfrom(ufd,udp_message,MAX_UPD_MESSAGE_LEN,0,(struct sockaddr *)&udp_useraddr,&addrlen);
                    if(ret>0){
                        if(strlen(udp_message)>0){
                            udp_message[ret-1]=0;
                        }
                        errcode=getnameinfo( (struct sockaddr *) &udp_useraddr,addrlen,host,sizeof host, service,sizeof service,0);
                        IP = inet_ntoa(udp_useraddr.sin_addr);
                        sprintf(client_port, "%ld", ntohs(udp_useraddr.sin_port));
                        read_message(udp_message);
                        udp_reply[strlen(udp_reply)] = 0;
                        sendto(ufd, udp_reply, strlen(udp_reply), 0, (struct sockaddr *) &udp_useraddr, addrlen);
                        memset(udp_reply, 0, strlen(udp_reply));
                        memset(udp_message, 0, strlen(udp_message));
                    }
                }
                if(FD_ISSET(tfd,&testfds)){
                    FILE *fp;
                    addrlen=sizeof(tcp_useraddr);
                    if((newfd=accept(tfd,(struct sockaddr*)&tcp_useraddr,&addrlen))==-1){
                        exit(1);
                    }
                    int spaces[8];
                    fds.fd = newfd;
                    fds.events = POLLIN;
                    thread = fork();
                    if(thread == 0){
                        IP = inet_ntoa(tcp_useraddr.sin_addr);
                        sprintf(client_port, "%ld", ntohs(tcp_useraddr.sin_port));
                        len = read_message_tcp(spaces, fds, newfd);
                        if(len == 0){
                            len = strlen(tcp_reply);
                            tcp_reply[len] = 0;
                            ptr = &tcp_reply[0];
                            while(len > 0){
                                copy = min(len, 256);
                                n = write(newfd, ptr, copy);
                                if(n==-1){
                                    perror("ERROR");
                                    exit(1);
                                }
                                ptr += n;
                                len -= n;
                            }
                        }
                        close(newfd);
                        memset(tcp_message, 0, strlen(tcp_message));
                        memset(tcp_reply, 0, strlen(tcp_reply));
                        exit(0);
                    }
                    else{
                        close(newfd);
                        while(wait(NULL)>0);
                    }
                }
        }
    }
}