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
#define TCPA_PORT 25061
#define TCPB_PORT 26061
#define UDP_PORT  24061
#define T_PORT 21061
#define S_PORT 22061
#define P_PORT 23061
#define localhost "127.0.0.1"
#define MAXMESSAGE 2048   //max size
#define BACKLOG 10  //pending connections queue

int sockfdA, sockfdB, sockfdUDP;;
struct sockaddr_in TCPaddrA, TCPaddrB;
struct sockaddr_in UDPaddr, UDPclientT, UDPclientS, UDPclientP;
int new_fda, new_fdb; 
struct sockaddr_in addrA,addrB;  //children sockets
char bufA[MAXMESSAGE], bufB[MAXMESSAGE];
char sendtoT[MAXMESSAGE], recvC[MAXMESSAGE], recvT[MAXMESSAGE], recvS[MAXMESSAGE], recvP[MAXMESSAGE];
char result[20000];

void createTCPsocket(){
    sockfdA = socket(AF_INET, SOCK_STREAM, 0);
    //create error happens
    if(sockfdA == ERROR){
        perror("client A socket TCP error");
        exit(1);
    }
    //bind: from Beej’s guide to network programming
    memset(&TCPaddrA, 0, sizeof TCPaddrA);
    TCPaddrA.sin_family = AF_INET;
    TCPaddrA.sin_port   = htons(25061);
    TCPaddrA.sin_addr.s_addr = inet_addr(localhost);

    if(::bind(sockfdA, (struct sockaddr*) &TCPaddrA, sizeof TCPaddrA ) == ERROR){
        perror("Client A TCP bind");
        exit(1);
    }


    sockfdB = socket(AF_INET, SOCK_STREAM, 0);
    //create error happens
    if(sockfdB == ERROR){
        perror("client B socket TCP error");
        exit(1);
    }
    //bind: from Beej’s guide to network programming
    memset(&TCPaddrB, 0, sizeof TCPaddrB);
    TCPaddrB.sin_family = AF_INET;
    TCPaddrB.sin_port   = htons(26061);
    TCPaddrB.sin_addr.s_addr = inet_addr(localhost);

    if(::bind(sockfdB, (struct sockaddr*) &TCPaddrB, sizeof TCPaddrB ) == ERROR){
        perror("clinet B TCP bind");
        exit(1);
    }
}

void createUDPsocket(){
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    //create error happens
    if(sockfdUDP == ERROR){
        perror("Central socket UDP error");
        exit(1);
    }
    
    //bind: from Beej’s guide to network programming
    memset(&UDPaddr, 0, sizeof UDPaddr);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port   = htons(UDP_PORT);
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
    createTCPsocket();
    createUDPsocket();

    // printf("The Central server is up and running."\n);
    cout << "The Central server is up and running." << endl;
    cout << endl;
    if (listen(sockfdA, BACKLOG) == -1) {
        perror("listen");
    }
    if (listen(sockfdB, BACKLOG) == -1) {
        perror("listen");
    }
    while(true){
        socklen_t sin_sizeA = sizeof addrA;	
        socklen_t sin_sizeB = sizeof addrB;	
        
        new_fda = accept(sockfdA, (struct sockaddr *) &addrA, &sin_sizeA);
        if (new_fda == -1) {
			perror("accept A error");			
			exit(1);
		}

        memset(bufA,'\0',sizeof bufA);
        if (recv(new_fda, bufA, sizeof bufA, 0) == -1) {
            perror("client A receive error");
            exit(1);
        }
        // printf("The Central server received input= \"%s\" from the client using TCP over port 25061.", bufA);
        cout << "The Central server received input= \"" << bufA << "\" from the client using TCP over port 25061." << endl;

        new_fdb = accept(sockfdB, (struct sockaddr *) &addrB, &sin_sizeB);
        if (new_fdb == -1) {
			perror("accept B error");			
			exit(1);
		}

        memset(bufB,'\0',sizeof bufB);
        if (recv(new_fdb, bufB, sizeof bufB, 0) == -1) {
            perror("client B receive error");
            exit(1);
        }
        // printf("The Central server received input= \"%s\" from the client using TCP over port 25061.", bufB);
        string recvfrB;
        for(int i = 1; i < strlen(bufB); i++){
            if(bufB[i] == '+') recvfrB += ' ';
            else recvfrB += bufB[i];
        }
        cout << "The Central server received input= \"" << recvfrB << "\" from the client using TCP over port 26061." << endl;
        // cout << bufB << endl;

        //get the number of names from clientB
        char name_num = bufB[0];

        //send two clients' name to server T
        memset(sendtoT,'\0',sizeof sendtoT);
        int i = 0, k = 0;
        while(i < strlen(bufA)) sendtoT[k++] = bufA[i++];
        sendtoT[k++] = '+';
        i = 1;
        while(bufB[i] != '+') sendtoT[k++] = bufB[i++];
        if(name_num == '2'){
            sendtoT[k++] = '+';
            i++;
            while(i < strlen(bufB)) sendtoT[k++] = bufB[i++];
        }
        // cout << sendtoT << endl;

        //"connect" with serverP: NOT a real connect
        UDPclientP.sin_family = AF_INET;
        UDPclientP.sin_addr.s_addr = inet_addr(localhost);
        UDPclientP.sin_port = htons(P_PORT);
        socklen_t UDPclientP_len = sizeof UDPclientP;
        //"connect" with serverT: NOT a real connect
        UDPclientT.sin_family = AF_INET;
        UDPclientT.sin_addr.s_addr = inet_addr(localhost);
        UDPclientT.sin_port = htons(T_PORT);
        //"connect" with serverS: NOT a real connect
        UDPclientS.sin_family = AF_INET;
        UDPclientS.sin_addr.s_addr = inet_addr(localhost);
        UDPclientS.sin_port = htons(S_PORT);

        //send two clients name to serverT
        if (sendto(sockfdUDP, sendtoT, sizeof sendtoT, 0, 
		    (const struct sockaddr *) &UDPclientT, (socklen_t)sizeof UDPclientT) == -1) {
			perror("central send serverT Error");
			exit(1);
		}
        cout << "The Central server sent a request to Backend-Server T." << endl;

        //receive the number of datagrams from serverT
        socklen_t UDPclientT_len = sizeof UDPclientT;
        memset(recvC, '\0', sizeof recvC);
        if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
			(struct sockaddr *) &UDPclientT, &UDPclientT_len) == -1) {
			perror("Receive From ServerT Error");
			exit(1);
		}
        //send the number of dataframs to serverP
        if (sendto(sockfdUDP, recvC, sizeof recvC, 0, 
            (const struct sockaddr *) &UDPclientP, (socklen_t)sizeof UDPclientP) == -1) {
            perror("central send serverP Error");
            exit(1);
        }

        string packs = "";
        for(int i = 0; i < strlen(recvC); i++){
            packs += recvC[i];
        }
        int packn = stoi(packs);


        //receive edgelist from serverT: small datagrams!
        for(int i = 0; i < packn; i++){
            memset(recvC,'\0',sizeof recvC);
            if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
                (struct sockaddr *) &UDPclientT, &UDPclientT_len) == -1) {
                perror("Receive From ServerT Error");
                exit(1);
            }
            // cout << recvC << endl;
            //send graph to serverP
            if (sendto(sockfdUDP, recvC, sizeof recvC, 0, 
                (const struct sockaddr *) &UDPclientP, (socklen_t)sizeof UDPclientP) == -1) {
                perror("central send serverP Error");
                exit(1);
            }
        }

        cout << "The Central server received information from Backend-Server T using UDP over port 24061." << endl;
        // cout << recvC << endl;


        //send two clients' name to server S
        if (sendto(sockfdUDP, sendtoT, sizeof sendtoT, 0, 
		    (const struct sockaddr *) &UDPclientS, (socklen_t)sizeof UDPclientS) == -1) {
			perror("central send serverS Error");
			exit(1);
		}
        cout << "The Central server sent a request to Backend-Server S" << endl;

        //receive the number of datagrams from serverS
        socklen_t UDPclientS_len = sizeof UDPclientS;
        memset(recvC, '\0', sizeof recvC);
        if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
			(struct sockaddr *) &UDPclientS, &UDPclientS_len) == -1) {
			perror("Receive From ServerT Error");
			exit(1);
		}
        //send the number of dataframs to serverP
        if (sendto(sockfdUDP, recvC, sizeof recvC, 0, 
            (const struct sockaddr *) &UDPclientP, (socklen_t)sizeof UDPclientP) == -1) {
            perror("central send serverP Error");
            exit(1);
        }

        packs = "";
        for(int i = 0; i < strlen(recvC); i++){
            packs += recvC[i];
        }
        packn = stoi(packs);

        //receive scores from serverS: small datagrams!
        for(int i = 0; i < packn; i++){
            memset(recvC,'\0',sizeof recvC);
            if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
                (struct sockaddr *) &UDPclientS, &UDPclientS_len) == -1) {
                perror("Receive From ServerS Error");
                exit(1);
            }
            // cout << recvC << endl;
            //send scores to serverP
            if (sendto(sockfdUDP, recvC, sizeof recvC, 0, 
                (const struct sockaddr *) &UDPclientP, (socklen_t)sizeof UDPclientP) == -1) {
                perror("central send serverP Error");
                exit(1);
            }
        }

        cout << "The Central server received information from Backend-Server S using UDP over port 24061." << endl;
        // cout << recvC << endl;

        //send two clients name to serverP
        if (sendto(sockfdUDP, sendtoT, sizeof sendtoT, 0, 
		    (const struct sockaddr *) &UDPclientP, (socklen_t)sizeof UDPclientP) == -1) {
			perror("central send serverS Error");
			exit(1);
		}
        cout << "The Central server sent a processing request to Backend-Server P" << endl;

        //receive the numer of result's datagram from serverP
        memset(recvC,'\0',sizeof recvC);
        if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
			(struct sockaddr *) &UDPclientP, &UDPclientP_len) == -1) {
			perror("Receive From ServerP Error");
			exit(1);
		}

        packs = "";
        for(int i = 0; i < strlen(recvC); i++){
            packs += recvC[i];
        }
        packn = stoi(packs);

        //receive edgelist from serverT: small datagrams!
        int pt = 0;
        memset(result, '\0', sizeof result);
        for(int i = 0; i < packn; i++){
            memset(recvC,'\0',sizeof recvC);
            if (recvfrom(sockfdUDP, recvC, sizeof recvC,0,
                (struct sockaddr *) &UDPclientP, &UDPclientP_len) == -1) {
                perror("Receive From ServerP Error");
                exit(1);
            }
            for(int i = 0; i < strlen(recvC); i++) result[pt++] = recvC[i];
        }

        cout << "The Central server received information from Backend-Server P using UDP over port 24061." << endl;

        // cout << result << endl;
        if (send(new_fda, result, sizeof result, 0) == -1) {
            perror("send to client A error");
            exit(1);
        }

        cout << "The Central server sent the results to client A." << endl;

        if (send(new_fdb, result, sizeof result, 0) == -1) {
            perror("send to client B error");
            exit(1);
        }

        cout << "The Central server sent the results to client B." << endl;

    }

    return 0;
}