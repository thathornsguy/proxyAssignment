#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <signal.h>
#include <fstream>
#include <vector>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include <numeric>

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
int globalSockfd;
vector<string> getChainfile(string fileName);
void awget(int ssIndex, vector<string> chainFile,string url,string fileName);
void deserialize(char *data, sendpacket* msgPacket);
void serialize(sendpacket* msgPacket, char *data);
string getAddresess(vector<string> addresses);
void printMsg(sendpacket* msgPacket);
void freemem();
void writeOut(vector<recvpacket*> packets,string fileName);
void printMsg(recvpacket* msgPacket);
void deserializer(char *data, recvpacket* msgPacket);
void serializer(recvpacket* msgPacket, char *data);
void sig_handler(int signal){
	close(globalSockfd);
	cout<<"\nClosing"<<endl;
	freemem();
	exit(0);
}
void serialize(sendpacket* msgPacket, char *data);


int main(int argc, char **argv)
{
	//prune arguments to make sure the correct num
	if(argc!=2&&argc!=4){
		cout << "Usage: awget <URL> [-c chainfile]"<<endl;
		return 5;
	}
	
	string url = argv[1];
	

	string chainfile = "chaingang.txt";
	if(argc == 4){
		int c;
		while ((c = getopt (argc, argv, "c:")) != -1)
			switch (c)
			{
			case 'c':
				chainfile = optarg;
			break;
			case '?':
				if (optopt == 'c'){
					cout << "Usage: awget <URL> [-c chainfile]"<<endl;
					return 4;
				}
				else{
					cout << "Usage: awget <URL> [-c chainfile]"<<endl;
					return 1;
				}
			default:
				cout << "Usage: awget <URL> [-c chainfile]" << endl;
				return 2;
		}
		if(chainfile.compare("")==0){
			cout << "Usage: awget <URL> [-c chainfile]" << endl;
			return 3;
		}
	}
	
	
	string fileName = "index.html";
	int index = url.find("/");
	//if url is in the format of google.com/file.html filename will be file.html
	//otherwise it will be index.html
	if(index != -1){
		string reverseUrl = url;
		reverse(begin(reverseUrl),end(reverseUrl));
		index = url.size()-reverseUrl.find("/")-1;
		fileName = url.substr(index+1,url.size());
	}	
	

	vector<string> chainFileContents = getChainfile(chainfile);
	
	//generates random number from 1 to number of lines in the file (don't want line 0)
	srand(time(NULL)); //seeding rand()
	int range = chainFileContents.size() - 1; 
	int ssIndex = (rand() % range) + 1; //index of ss from chaingang.txt
	cout << "p.addresses[" << ssIndex <<"]: " << chainFileContents[ssIndex] << endl; 
	
	//socket stuff (random index generated and packet sent)
	
	
	awget(ssIndex, chainFileContents,url,fileName);
	freemem();
	return 0;
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

vector<string> getChainfile(string fileName){
	vector <string> addresses;
	ifstream file;
	file.open(fileName);
	string line;
	if(file.is_open()){
		while(getline(file,line)){
			addresses.push_back(line);
		}
		file.close();
	}else{
		cout << "Error: file not found" << endl;
	}
	

	return addresses;
}

void awget(int ssIndex, vector<string> chainFile,string url,string fileName)
{	
	//parsing the chosen line and obtaining IP and port
	string ipAddress;
	int port;

	string buffer;
	stringstream str(chainFile[ssIndex]);
	vector<string> tokens; 
	
	while ((str >> buffer))
	{	
		tokens.push_back(buffer);
	}

	ipAddress = tokens[0]; 
	port = stoi(tokens[1]); 
	
	cout << "ipAddress: " << ipAddress << endl;
	cout << "port: " << port << endl;
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	server.sin_port = htons(port);

	//make socket
	globalSockfd = socket(AF_INET,SOCK_STREAM,0);
	
	if(globalSockfd < 0){
		cout << "Error opening socket.\n";
		exit(8);
	}
	
	//connect to the server
	int connectClient = connect(globalSockfd, (struct sockaddr*)&server, sizeof(server));

	if(connectClient < 0){
		cout << "Error connecting.\n";
		exit(9);
	}else{cout << "Connected!\n";} //for debug purposes
	

	//strip the ss from the chainlist (i.e the vector in packet and send packet to ss)
	//the smaller chainlist means that now each ss has new indices. Is this what it should do?
	
	
	chainFile.erase(chainFile.begin()+ssIndex);
	
	
	
	//updating the first line, needs to be a string
	string newSize = to_string((chainFile.size()-1));
	
	struct sendpacket* p = new sendpacket;
	toFree.push_back(p);
	p->url=(char*)url.c_str();
	string tempAddresses = getAddresess(chainFile);
	p->addresses=(char*)tempAddresses.c_str();
	p->addresses[0]=newSize.c_str()[0];
	p->urlSize = strlen(p->url);
	p->addressesSize=strlen(p->addresses);
	cout << "\nFile AFTER removing ss:" << endl;
	char data[1000];
	printMsg(p);
	serialize(p,data);
	if( send(globalSockfd , data , 1000 , 0) < 0)
	{
		puts("Error: send failed.");
		sig_handler(-1);
	}
	//TODO: send the packet to the ss
	int numberOfPackets = 2; 
	bool first = true;
	vector<recvpacket*> packets;
	while(numberOfPackets > 0){
		cout << "received"<<endl;
		struct recvpacket* r = new recvpacket;
		toFree.push_back(r);
		int j=recv(globalSockfd , (recvpacket*)r, sizeof(recvpacket), MSG_WAITALL);
		if(j <=0){
			puts("recv failed");
			sig_handler(-1);
		}
		cout <<"Bytes Received: " <<j <<endl;
		if(first){
			numberOfPackets = r->numpackets;
			first = false;
		}
		printMsg(r);
		packets.push_back(r);
		numberOfPackets--;
		cout << "received packet! "<<numberOfPackets<<endl;
		
	}
	
	writeOut(packets,fileName);
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

void writeOut(vector<recvpacket*> packets,string fileName){
	vector<string> webpage((unsigned int)packets[0]->numpackets);
	for(unsigned int i =0; i < (unsigned int)packets[0]->numpackets;i++){
		cout << packets[i]->seqno << endl;
		webpage[packets[i]->seqno]=packets[i]->page;
	}
	string totalPage;
	for(unsigned int i =0; i < webpage.size();i++){
		totalPage += webpage[i];
	}
	ofstream file;
	file.open(fileName);
	file << totalPage;
	file.close();
}

void printMsg(recvpacket* msgPacket)
{
    cout << "seqno: "<<msgPacket->seqno << endl;
    cout << "numpackets: "<<msgPacket->numpackets << endl;
    cout << "page: "<<msgPacket->page << endl;

}
