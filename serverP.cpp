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
#include <cmath>

using namespace std;


#define ERROR -1
#define TCPA_PORT 25061
#define TCPB_PORT 26061
#define UDP_PORT  24061
#define P_PORT 23061
#define localhost "127.0.0.1"
#define MAXMESSAGE 1000   //max size
#define BACKLOG 10  //pending connections queue
#define inf 1e8

typedef pair<string, double> PSD;
typedef pair<double, string> PDS;

int sockfdUDP;
struct sockaddr_in UDPaddr, UDPclientC;
char recvP[MAXMESSAGE], sendP[1000000];
char recvC[1000000], sendC[MAXMESSAGE];

// unordered_map<string, int> name_to_num;
unordered_map<string, vector<PSD> > edges; 
unordered_map<string, int> scores;
unordered_map<string, double> dist; 
unordered_map<string, string> prevto;
unordered_map<string, bool> state;

void createUDPsocket(){
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    //create error happens
    if(sockfdUDP == ERROR){
        perror("Central socket UDP error");
        exit(1);
    }
    
    //bind : from Beej
    memset(&UDPaddr, 0, sizeof UDPaddr);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port   = htons(P_PORT);
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

double calclength(string a, string b){
    if((!scores.count(a)) || (!scores.count(b))) return 1e8;

    int sa = scores[a], sb = scores[b];
    return fabs(sa-sb) / (sa+sb);
}

void calcpath(string a, string b){
    for(auto& p : scores){
        auto& k = p.first;
        auto& v = p.second;
        dist[k] = inf;
        state[k] = false;
    }

    dist[a] = 0;
    prevto[a] = "end";
    priority_queue<PDS, vector<PDS>, greater<PDS> > pq;
    pq.push(PDS(0, a));

    while(pq.size()){
        auto t = pq.top();
        pq.pop();
        double dis = t.first;
        string ver = t.second;

        // if(ver == b) break;

        if(!state[ver]){
            state[ver] = true;
            // cout << "ver = " << ver << endl;

            for(auto p : edges[ver]){
                string to = p.first;
                double lg = p.second;
                if(dist[to] > dist[ver] + lg){
                    dist[to] = dist[ver] + lg;
                    prevto[to] = ver;
                    pq.push(PDS(dist[to], to));
                }
            }
        }

    }

    int k = 3;
    
    if(dist[b] > inf / 2){
        sendP[0] = '@';
        for(int i = 0; i < a.size(); i++) sendP[k++] = a[i];
        sendP[k++] = '+';
        for(int i = 0; i < b.size(); i++) sendP[k++] = b[i];
        sendP[k++] = '+';
    }
    else{
        auto distance_o = dist[b];
        char distance[100];
        memset(distance, '\0', sizeof distance);
        sprintf(distance, "%.2f", distance_o);
        int i = 0;
        while(i < strlen(distance)){
            sendP[k++] = distance[i++];
        }
        sendP[k++] = '*';

        string start = b;
        while(start != "end"){
            int j = 0;
            while(j < start.size()) sendP[k++] = start[j++];
            sendP[k++] = '+';
            start = prevto[start];
        }
    }

    sendP[k++] = '#';
    // cout << "sendp = " << sendP << endl;
}


int main(){
    createUDPsocket();

    cout << "The ServerP is up and running using UDP on port 23061." << endl;

    while(true){
        UDPclientC.sin_family = AF_INET;
        UDPclientC.sin_addr.s_addr = inet_addr(localhost);
        UDPclientC.sin_port = htons(UDP_PORT);

        socklen_t UDPclientC_len = sizeof UDPclientC;

        //receive the number of datagrams from central: edges
        memset(recvP,'\0',sizeof recvP);
        if (recvfrom(sockfdUDP,recvP, sizeof recvP,0,
			(struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
			perror("serverP Receive Error");
			exit(1);
		}

        string packs = "";
        for(int i = 0; i < strlen(recvP); i++){
            packs += recvP[i];
        }
        int packn = stoi(packs);

        //receive graph from central
        int pt = 0;
        memset(recvC, '\0', sizeof recvC);
        for(int i = 0; i < packn; i++){
            memset(recvP,'\0',sizeof recvP);
            if (recvfrom(sockfdUDP,recvP, sizeof recvP,0,
                (struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
                perror("serverP Receive Error");
                exit(1);
            }
            //copy small datagrams into a complete datagram
            for(int j = 0; j < strlen(recvP); j++) recvC[pt++] = recvP[j];
            // cout << recvP << endl;
        }

        //build the graph
        int i = 0;
        string a,b;
        bool second = false;
        while(i < strlen(recvC)){
            if(second){
                if(recvC[i] == '*'){
                    edges[a].push_back(PSD(b, inf));
                    edges[b].push_back(PSD(a, inf));
                    second = false;
                    a = "";
                    b = "";
                    i++;
                }
                else{
                    b += recvC[i++];
                }
            }
            else{
                if(recvC[i] == '+'){
                    second = true;
                    i++;
                }
                else{
                    a += recvC[i++];
                }
            }
        }

        for(auto& p : edges){
            auto& k = p.first;
            auto& v = p.second;
            sort(v.begin(), v.end());
            v.erase(unique(v.begin(), v.end()), v.end());
        }



        //receive the number of datagrams from central: scores
        memset(recvP,'\0',sizeof recvP);
        if (recvfrom(sockfdUDP,recvP, sizeof recvP,0,
			(struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
			perror("serverP Receive Error");
			exit(1);
		}

        packs = "";
        for(int i = 0; i < strlen(recvP); i++){
            packs += recvP[i];
        }
        packn = stoi(packs);

        //receive scores from central
        pt = 0;
        memset(recvC, '\0', sizeof recvC);
        for(int i = 0; i < packn; i++){
            memset(recvP,'\0',sizeof recvP);
            if (recvfrom(sockfdUDP,recvP, sizeof recvP,0,
                (struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
                perror("serverP Receive Error");
                exit(1);
            }
            //copy small datagrams into a complete datagram
            for(int j = 0; j < strlen(recvP); j++) recvC[pt++] = recvP[j];
            // cout << recvP << endl;
        }

        //store all the scores
        i = 0;
        a = "";
        b = "";
        second = false;
        while(i < strlen(recvC)){
            if(second){
                if(recvC[i] == '*'){
                    scores[a] = stoi(b);
                    i++;
                    second = false;
                    a = "";
                    b = "";
                }
                else{
                    b += recvC[i++];
                }
            }else{
                if(recvC[i] == '+'){
                    second = true;
                    i ++;
                }
                else{
                    a += recvC[i++];
                }
            }
        }

        //calculate "length" of each edge
        for(auto& p : edges){
            auto& k = p.first;
            auto& v = p.second;
            a = k;
            for(auto& p: v){
                b = p.first;
                p.second = calclength(a,b);
            }
        }

        //receive names from central
        memset(recvP,'\0',sizeof recvP);
        if (recvfrom(sockfdUDP,recvP, sizeof recvP,0,
			(struct sockaddr *) &UDPclientC, &UDPclientC_len) == -1) {
			perror("serverP Receive Error");
			exit(1);
		}
        // cout << recvP << endl;

        cout << "The ServerP received the topology and score information." << endl;

        i = 0;
        a = "";
        b = "";
        string c = "";
        second = false;
        bool third = false;
        while(i < strlen(recvP)){
            if(third){
                c += recvP[i++];
            }
            else if(second){
                if(recvP[i] == '+'){
                    third = true;
                    i++;
                }
                else b += recvP[i++];
            }
            else{
                if(recvP[i] == '+'){
                    second = true;
                    i++;
                }
                else a += recvP[i++];
            }
        }
        dist[b] = inf;
        state[b] = false;
        if(c.size()){
            dist[c] = inf;
            state[c] = false;
        }

        //'$' == good, '@' == bad
        //'Y' == two paths, 'N' = one paths;
        memset(sendP,'\0',sizeof sendP);
        bool two_path = c.size()? true : false;
        bool A,B,C;
        A = scores.count(a);
        B = scores.count(b);
        C = scores.count(c);
        if(!A){
            sendP[0] = '@';
            sendP[1] = two_path? 'Y' : 'N';
            sendP[2] = '@';
            int k = 3;
            for(int i = 0; i < a.size(); i++) sendP[k++] = a[i];
            sendP[k++] = '+';
            for(int i = 0; i < b.size(); i++) sendP[k++] = b[i];
            sendP[k++] = '+';
            sendP[k++] = '#';
            if(two_path){
                for(int i = 0; i < a.size(); i++) sendP[k++] = a[i];
                sendP[k++] = '+';
                for(int i = 0; i < c.size(); i++) sendP[k++] = c[i];
                sendP[k++] = '+';
            }
        }
        else{
            sendP[0] = '$';
            sendP[1] = two_path? 'Y' : 'N';
            sendP[2] = '$';
            calcpath(a,b);
            if(two_path){
                int k = 0;
                while(sendP[k] != '#') k++;
                k++;
                sendP[1] = 'Y';
                if(dist[c] > inf / 2){
                    sendP[2] = '@';
                    for(int i = 0; i < a.size(); i++) sendP[k++] = a[i];
                    sendP[k++] = '+';
                    for(int i = 0; i < c.size(); i++) sendP[k++] = c[i];
                    sendP[k++] = '+';
                }
                else{
                    auto distance_o = dist[c];
                    char distance[100];
                    memset(distance, '\0', sizeof distance);
                    sprintf(distance, "%.2f", distance_o);
                    int i = 0;
                    while(i < strlen(distance)){
                        sendP[k++] = distance[i++];
                    }
                    sendP[k++] = '*';

                    string start = c;
                    while(start != "end"){
                        int j = 0;
                        while(j < start.size()) sendP[k++] = start[j++];
                        sendP[k++] = '+';
                        start = prevto[start];
                    }
                }
            }
        }

        // cout << "a == " << a << " c == " <<  c << endl;
        // cout << "dist[c] == " << dist[c] << endl;
        //send result to central
        // cout << sendP << endl;
        int sendsz = strlen(sendP);
        int sendnum = (sendsz + 899) / 900;
        char length[100];
        memset(length, '\0', sizeof length);
        string size_P = to_string(sendnum);
        strcpy(length, size_P.c_str());
        //send the number of datagrams to central
        if (sendto(sockfdUDP, length, sizeof length, 0, 
		    (const struct sockaddr *) &UDPclientC, (socklen_t)sizeof UDPclientC) == -1) {
			perror("ServerP Response Error");
			exit(1);
		}
        //send each small datagram to central
        pt = 0;
        for(int i = 0; i < sendnum; i++){
            //get each small datagram
            memset(sendC, '\0', sizeof sendC);
            for(int j = 0; j < 900; j++){
                sendC[j] = sendP[pt++];
                if(pt == sendsz) break;
            }
            //send datagram to cenral
            if (sendto(sockfdUDP, sendC, sizeof sendC, 0, 
                (const struct sockaddr *) &UDPclientC, (socklen_t)sizeof UDPclientC) == -1) {
                perror("ServerS Response Error");
                exit(1);
            }
        }

        cout << "The ServerP finished sending the results to the Central." << endl;
    }

    close(sockfdUDP);
    return 0;
}