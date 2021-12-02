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
#include <fstream>
#include <cassert>

using namespace std;


#define ERROR -1
#define TCPA_PORT 25061
#define TCPB_PORT 26061
#define UDP_PORT  24061
#define T_PORT 21061
#define localhost "127.0.0.1"
#define MAXMESSAGE 1000000   //max size
#define BACKLOG 10  //pending connections queue

int sockfdUDP;
struct sockaddr_in UDPaddr, UDPclientC;
char recvT[MAXMESSAGE], sendT[MAXMESSAGE], sendC[1000];

void createUDPsocket(){
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    //create error happens
    if(sockfdUDP == ERROR){
        perror("Central socket UDP error");
        exit(1);
    }
    
    //bind: from beej
    memset(&UDPaddr, 0, sizeof UDPaddr);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port   = htons(T_PORT);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);

    if(::bind(sockfdUDP, (struct sockaddr*) &UDPaddr, sizeof UDPaddr ) == ERROR){
        perror("clinet B TCP bind");
        exit(1);
    }
}

//get_in_addr: from Beej’s guide to network programming
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(){
    createUDPsocket();

    cout << "The ServerT is up and running using UDP on port 21061." << endl;

    while(true){
        UDPclientC.sin_family = AF_INET;
        UDPclientC.sin_addr.s_addr = inet_addr(localhost);
        UDPclientC.sin_port = htons(UDP_PORT);

        socklen_t UDPclientC_len = sizeof UDPclientC;

        if (recvfrom(sockfdUDP,recvT, sizeof recvT,0,
			(struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
			perror("serverT Receive Error");
			exit(1);
		}

        cout << "The ServerT received a request from Central to get the topology." << endl;
        // cout << recvT << endl;

        //from guidebook
        ifstream infile; 
        infile.open("./edgelist.txt");
        if(!infile.is_open()){
            std::cout<< "open edgelist.txt error" << endl;
            exit(1);     
        }
        string s;
        int k = 0;
        while(getline(infile, s)){
            int i = 0;
            while(i < s.size()){
                if(s[i] == ' '){
                    sendT[k++] = '+';
                    i++;
                }
                else sendT[k++] = s[i++];
            } 
            sendT[k++] = '*';
        }
        infile.close(); 


        //divide large array to smaller ones
        int sizeT = strlen(sendT);
        int packs = (sizeT + 899) / 900;  //send times
        char length[100];
        memset(length, '\0', sizeof length);
        string size_P = to_string(packs);
        strcpy(length, size_P.c_str());
        //send the number of datagrams to central
        if (sendto(sockfdUDP, length, sizeof length, 0, 
		    (const struct sockaddr *) &UDPclientC, (socklen_t)sizeof UDPclientC) == -1) {
			perror("ServerT Response Error");
			exit(1);
		}
        int pt = 0;
        for(int i = 0; i < packs; i++){
            //get each small datagram
            memset(sendC, '\0', sizeof sendC);
            for(int j = 0; j < 900; j++){
                sendC[j] = sendT[pt++];
                if(pt == sizeT) break;
            }
            //send datagram to cenral
            if (sendto(sockfdUDP, sendC, sizeof sendC, 0, 
                (const struct sockaddr *) &UDPclientC, (socklen_t)sizeof UDPclientC) == -1) {
                perror("ServerT Response Error");
                exit(1);
            }
        }

        cout << "The ServerT finished sending the topology to Central." << endl;
    }

    close(sockfdUDP);
    return 0;
}