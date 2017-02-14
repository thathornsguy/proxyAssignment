#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <pthread.h>
#include <fstream>
#include <math.h>  
#include <sstream>

using namespace std;

#define MAX 1200
typedef struct recvpacket {
	short seqno;
	short numpackets;
	char page[MAX];
	
}recvpacket;

typedef struct sendpacket {
	short urlSize;
	short addressesSize;
	char* url;
	char* addresses;
}sendpacket;
vector<void*> toFree;
int globalSockfdIn;
int globalSockfdOut;
void ss(int port);


string getAddresess(vector<string> addresses);
void deserialize(char *data, sendpacket* msgPacket);
void serialize(sendpacket* msgPacket, char *data);
void printMsg(sendpacket* msgPacket);
vector<string> portAddresses(string addresses);
void freemem();
void receiveStuff();
void* threadedProcess(void* useless);
string readWeb();
vector<recvpacket*> makeRecvPacket(string web);
void printMsg(recvpacket* msgPacket);
void deserializer(char *data, recvpacket* msgPacket);
void serializer(recvpacket* msgPacket, char *data);
void sig_handler(int signal){
	close(globalSockfdIn);
	cout<<"\nClosing"<<endl;
	freemem();
	pthread_exit (0);
	exit(0);
}
int main(int argc, char **argv)
{
	signal(SIGINT,sig_handler);
	int port = 5011; 
	if(argc == 3){
		int c;
		while ((c = getopt (argc, argv, "p:")) != -1)
			switch (c)
			{
			case 'p':
				port = atoi(optarg);
				if(port < 0 || port > 65535){
					cout << "Error: Invalid Port"<<endl;
					return 3;
				}
			break;
			case '?':
				if (optopt == 'p'){
					cout << "Usage: ss -p port"<<endl;
					return 4;
				}
				else{
					cout << "Usage: ss -p port"<<endl;
					return 1;
				}
			default:
				cout << "Usage: ss -p port" << endl;
				return 2;
		}
	}else if(argc != 1){
		cout << "Usage: ss -p port" << endl;
		return 5;
	}
	
	ss(port); 
	freemem();
	close(globalSockfdIn);
	return 0;
}

void ss (int port){
	
	struct sockaddr_in server;
	char hostName[50]; 
	gethostname(hostName, 50); 
	struct hostent *h = gethostbyname(hostName); 
	
	char strptr [INET_ADDRSTRLEN];
	inet_ntop(AF_INET, (struct in_addr*)h->h_addr, strptr, INET_ADDRSTRLEN);
	
	//begin network shenanagins
	cout << "Listening on "<<strptr<<" and port "<<port << endl;
	globalSockfdIn = socket(AF_INET,SOCK_STREAM,0);
	if(globalSockfdIn < 0){
		cout << "Error opening socket.\n";
		exit(8);
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(strptr);
	server.sin_port = htons(port);
	
	//bind to socket
	int bindNum = bind(globalSockfdIn,(struct sockaddr *)&server, sizeof(server));
	
	if(bindNum < 0){
		
		cout << "Error: bind failed\n";
		exit(7);
	}
	
	//begin listening
	listen(globalSockfdIn,5);
	//~ while(1){
	//~ begin thread
		//~ pthread_t tid;		
		//~ pthread_attr_t attr;
	
		//~ pthread_attr_init (&attr);
		//~ void* useless;
		//~ pthread_create (&tid, &attr, threadedProcess, useless);
	receiveStuff();
		//~ pthread_join (tid, NULL);
	//~ }
}

vector<string> portAddresses(string addresses){
vector<string> toReturn;
string delimiter = ":";

size_t pos = addresses.find(delimiter);
string token;
do {
    token = addresses.substr(0, pos);
    toReturn.push_back(token);
    addresses.erase(0, pos + delimiter.length());
}while ((pos = addresses.find(delimiter)) != string::npos);
 toReturn.push_back(addresses);
return toReturn;
}

void serialize(sendpacket* msgPacket, char *data)
{
    short *q = (short*)data;    
    *q = msgPacket->urlSize;       q++;    
    *q = msgPacket->addressesSize;   q++;    
	
    char *p = (char*)q;
    int i = 0;
    while (i < msgPacket->urlSize)
    {
        *p = msgPacket->url[i];
        p++;
        i++;
    }
    i=0;
    while (i < msgPacket->addressesSize)
    {
        *p = msgPacket->addresses[i];
        p++;
        i++;
    }
}

void deserialize(char *data, sendpacket* msgPacket)
{
    short *q = (short*)data;    
    msgPacket->urlSize = *q;       q++;    
    msgPacket->addressesSize = *q;   q++;    
	msgPacket->url=(char*)calloc(1,msgPacket->urlSize);
	msgPacket->addresses=(char*)calloc(1,msgPacket->addressesSize);
	toFree.push_back(msgPacket->url);
	toFree.push_back(msgPacket->addresses);	
    char *p = (char*)q;
    int i = 0;
    while (i < msgPacket->urlSize)
    {
        msgPacket->url[i] = *p;
        p++;
        i++;
    }
    i=0;
    while (i < msgPacket->addressesSize)
    {
        msgPacket->addresses[i]=*p ;
        p++;
        i++;
    }
}


void printMsg(sendpacket* msgPacket)
{
    cout << "url: "<<msgPacket->url << endl;
    cout << "urlsize: "<<msgPacket->urlSize << endl;
    cout << "addresses: "<<msgPacket->addresses << endl;
    cout << "addressessize: "<<msgPacket->addressesSize << endl;
}

void freemem(){
	for(unsigned int i = 0; i < toFree.size();i++){
		free(toFree[i]);
	}
}

void* threadedProcess(void* useless){
	
	//accept connection from other client
	receiveStuff();
	pthread_exit (0);
}

void receiveStuff(){
struct sockaddr_in client;
socklen_t c = sizeof(client);
int clientCheck = accept(globalSockfdIn, (struct sockaddr *)&client, &c);
	
	if(clientCheck < 0){
		
		cout << "Error: accept failed\n";
		exit(6);
	}else{cout << "Accepted!\n";} //for debug purposes
	
	char data[1000];
	if(recv(clientCheck , data, 1000, 0)<0){
		puts("recv failed");
		sig_handler(-1);
	}
	cout<<strlen(data)<<endl;
	struct sendpacket* p = new sendpacket;
	toFree.push_back(p);
	deserialize(data,p);
	printMsg(p);
	vector<string>addresses = portAddresses(p->addresses);

	if(p->addresses[0]=='0'){
			//get file
			cout << "getting file" << endl;
			system(("wget "+(string)p->url+" -O index").c_str());
			string webpage=readWeb();
			
			
			vector<recvpacket*> packets = makeRecvPacket(webpage);
			
			for(unsigned int i = 0; i < packets.size();i++){
				printMsg(packets[i]);
				int sent = write(clientCheck,(char*)packets[i],sizeof(recvpacket));
				cout <<"Bytes sent:" <<sent << endl;
			}

			
			cout << "done sending" << endl;
			
	}else{
		cout << "going to ss" << endl;
		
		srand(time(NULL)); //seeding rand()
		int range = addresses.size() - 1; 
		int ssIndex = (rand() % range) + 1; //index of ss from chaingang.txt
		cout << range << endl;
		cout << ssIndex << endl;
		
		
		
		
		string ipAddress;
		int port;
		
		string buffer;
		cout << addresses[1]<<endl;
		stringstream str(addresses[ssIndex]);
		vector<string> tokens; 
		
		while ((str >> buffer))
		{	
			tokens.push_back(buffer);
		}
		cout << tokens.size()<<endl;
		ipAddress = tokens[0]; 
		port = stoi(tokens[1]); 
		
		cout << "ipAddress: " << ipAddress << endl;
		cout << "port: " << port << endl;
		
		addresses.erase(addresses.begin()+ssIndex);
		
		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr(ipAddress.c_str());
		server.sin_port = htons(port);
		
		globalSockfdOut = socket(AF_INET,SOCK_STREAM,0);
	
		if(globalSockfdOut < 0){
			cout << "Error opening socket.\n";
			exit(8);
		}
		
		int connectClient = connect(globalSockfdOut, (struct sockaddr*)&server, sizeof(server));
		if(connectClient < 0){
		cout << "Error connecting.\n";
		exit(9);
		}else{cout << "Connected!\n";} //for debug purposes
		string newSize = to_string((addresses.size()-1));
		
		struct sendpacket* newPacket = new sendpacket;
		newPacket->urlSize = p->urlSize;
		newPacket->addressesSize = p->addressesSize;
		string tempAddresses = getAddresess(addresses);
		newPacket->url = p->url;
		newPacket->addresses = (char*)tempAddresses.c_str();
		newPacket->addresses[0]=newSize.c_str()[0];
		cout<<"SENDING"<<endl;
		printMsg(newPacket);

		char data[1000];
		serialize(newPacket,data);
		
		
		
		
		if( send(globalSockfdOut , data , 1000 , 0) < 0)
		{
			puts("Error: send failed.");
			sig_handler(-1);
		}
		
		int numberOfPackets = 2; 
		bool first = true;
		vector<recvpacket*> packets;
		while(numberOfPackets > 0){
			cout << "received"<<endl;
			struct recvpacket* r = new recvpacket;
			toFree.push_back(r);
			if(recv(globalSockfdOut , (recvpacket*)r, sizeof(recvpacket), MSG_WAITALL)<0){
				puts("recv failed");
				sig_handler(-1);
			}
			if(first){
				numberOfPackets = r->numpackets;
				first = false;
			}
			printMsg(r);
			packets.push_back(r);
			numberOfPackets--;
			cout << "received packet! "<<numberOfPackets<<endl;
			
		}
		
		for(unsigned int i = 0; i < packets.size();i++){
				printMsg(packets[i]);
				int sent = write(clientCheck,(char*)packets[i],sizeof(recvpacket));
				cout <<"Bytes sent:" <<sent << endl;
		}
	}
}




string readWeb(){
	string toReturn;
	ifstream file;
	file.open("index");
	string line;
	if(file.is_open()){
		while(getline(file,line)){
			toReturn+=line;
		}
		file.close();
	}else{
		cout << "Error: file not found" << endl;
	}
	system("rm index");
	return toReturn;	
}

string getAddresess(vector<string> addresses){
	string toReturn = (char*)"";
	for(unsigned int i = 0; i < addresses.size();i++){
		if(i==0){
			toReturn = toReturn + addresses[i];
		}else{
			toReturn += ":"+addresses[i]; 
		}
	}
	return toReturn;
}

vector<recvpacket*> makeRecvPacket(string web){
	unsigned int numPackets = ceil(strlen(web.c_str())/(double)MAX);

	vector<recvpacket*> toReturn;
	for(unsigned int i = 0; i < numPackets; i++){
		cout << "i: " << i << endl;
		struct recvpacket* cur = new recvpacket;
		toFree.push_back(cur);
		cur->seqno = i;
		cur->numpackets = numPackets;
		if(web.size() >= MAX){	
		
		strcpy(cur->page,(char*)web.substr(0,MAX).c_str());	
		web.erase(0,MAX);
		}else{
		
		strcpy(cur->page,(char*)web.substr(0,web.size()).c_str());	
		}
		toReturn.push_back(cur);
	}
	return toReturn;
}

void printMsg(recvpacket* msgPacket)
{
    cout << "seqno: "<<msgPacket->seqno << endl;
    cout << "numpackets: "<<msgPacket->numpackets << endl;
    cout << "page: "<<msgPacket->page << endl;

}
