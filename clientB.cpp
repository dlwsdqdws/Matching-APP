#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <deque>
#include <algorithm>
#include <tuple>
#include <sys/types.h>
#include <netdb.h>

using namespace std;

#define ERROR -1
#define localhost "127.0.0.1"
#define TCPA_PORT "25061"
#define TCPB_PORT "26061"
#define Maxs 200000   //max size
#define TCPA_PORT "25061"

struct sockaddr_in clientAadd;

char input[Maxs], output[Maxs];
int sockfd;
char username[2048];

void createTCPsocket(){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //create error happens
    if(sockfd == ERROR){
        perror("clientA socket TCP error");
        exit(1);
    }
}

void TCPconnect(){
    //from Beej’s guide to network programming
    memset(&clientAadd, 0, sizeof clientAadd);
    clientAadd.sin_family = AF_INET;  //ipv4
    clientAadd.sin_addr.s_addr = inet_addr(localhost);
    clientAadd.sin_port = htons(26061);

    //tcp connect
    if((connect(sockfd,(struct sockaddr *)&clientAadd, sizeof clientAadd)) == ERROR){
        perror("Connecting Error");
        exit(1);
    }
}

int main(int argc, char* argv[]){
    memset(username, '\0', sizeof username);
    username[0] = '1';
    strcpy(username+1, argv[1]);  //get the username
    int sz = strlen(username);
    if(argc == 3){
        username[sz++] = '+';
        int j = 0;
        while(argv[2][j]){
            username[sz++] = argv[2][j++];
        }
        username[0] = '2';
    }
        
    createTCPsocket();
    TCPconnect();
    printf("The client is up and running.\n\n");

    if(send(sockfd, username, sizeof username, 0) == ERROR){
        perror("client B send error");
        exit(1);
    }

    string sendtoC;
    for(int i = 1; i < strlen(username); i++){
        if(username[i] == '+') sendtoC+= ' ';
        else sendtoC += username[i];
    }
    printf("The client sent \"%s\" to the Central server.\n", sendtoC.c_str());

    //receive the result from central
    memset(output,'\0',sizeof output);
    if(recv(sockfd, output, sizeof output, 0) == ERROR){
        perror("client B receive error");
        exit(1);
    }

    bool st1 = output[0] == '$'? true : false;
    bool st2 = output[2] == '$'? true : false;
    bool two_path = output[1] == 'Y'? true : false;

    //decode the result
    string score = "", state = "", score2 = "";
    int i = 3;
    vector<string>  paths, paths2;
    paths.clear();
    paths2.clear();

    if(st1){
        while(i < strlen(output) && output[i] != '*') score += output[i++];
        i++;
    }
    while( i < strlen(output) && output[i] != '#'){
        if(output[i] == '+'){
            paths.push_back(state);
            state = "";
        }
        else state += output[i];
        i++;
    }

    i++;  //skip '#'
    if(two_path){
        if(st2){
            while(i < strlen(output) && output[i] != '*') score2 += output[i++];
            i++;
        }
        while(i < strlen(output)){
            if(output[i] == '+'){
                paths2.push_back(state);
                state = "";
            }
            else state += output[i];
            i++;
            }
    }
    
    //print the result
    string result = "";
    if(!st1) cout << "Found no compatibility between " << paths.back() << " and " << paths[0] << endl;
    else{
        cout << "Found compatibility for " << paths[0] <<  " and " << paths.back() << ":" << endl;
        for(int i = 0; i < paths.size(); i++){
            result += paths[i];
            result += " --- ";
        }
        result.pop_back();
        result.pop_back();
        result.pop_back();
        result.pop_back();
        result.pop_back();
        cout << result << endl;
        cout << "Matching Gap: " << score << endl;
    }


    //print the result : second path
    if(two_path){
        if(!st2) cout << "Found no compatibility between " << paths2.back() << " and " << paths2[0] << endl;
        else{
            cout << "Found compatibility for " << paths2[0] <<  " and " << paths2.back() << ":" << endl;
            result = "";
            for(int i = 0; i < paths2.size(); i++){
                result += paths2[i];
                result += " --- ";
            }
            result.pop_back();
            result.pop_back();
            result.pop_back();
            result.pop_back();
            result.pop_back();
            cout << result << endl;
            cout << "Matching Gap: " << score2 << endl;
        }
    }

    close(sockfd);

    return 0;
}