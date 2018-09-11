// socket.cpp : definisce il punto di ingresso dell'applicazione console.
//

#include "stdafx.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <ctime>
#include <iostream>
#include <string>
#include <time.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <map>
#include <mutex>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "1500"

//credenziali per database mysql
#include <mysql.h> 
#include <sql.h>
#define SERVER "den1.mysql3.gear.host"
#define USER "pdsprojectdb"
#define PASSWORD "Kq4q?99Ngw!m"
#define DATABASE "pdsprojectdb"


using namespace std;

int ID = 0;

map<string, int> MacID;
int fine_ricezione = 0;
mutex m;
condition_variable cv;

typedef struct {
	int ID;
	string hash;
	unsigned channel; //: 4;
	signed rssi; //: 8;
	unsigned seqNumber;
	string/*long int*/ SSID;//a caso
							//uint8_t addr2[6]; /* sender address */
	string addr2;
	string timestamp;
} data_received;


bool readLine(data_received *dataR, string const &line)
{

	stringstream str_parse(line);

	string date;
	string hour;

	hash<string> h;
	//dataR->hash = h(line); //HASH DELLE INFO DEL PACCHETTO O DI TUTTO IL PACCHETTO?
						  //id >> dataR.ID;
	dataR->hash = to_string(rand());
	//dataR->hash = "hashdelpacchetto";//remove
	str_parse >> dataR->channel;
	if (!str_parse.good()) { std::cerr << "invalid channel\n";  return 1; }
	//if (str_parse.get() != ':') { std::cerr << "expected colon\n"; return false; }

	str_parse >> dataR->rssi;
	if (!str_parse.good()) { std::cerr << "invalid rssi\n"; return 1; }
	str_parse >> dataR->seqNumber;
	if (!str_parse.good()) { std::cerr << "invalid seqNumber\n"; return 1; }
	str_parse >> dataR->SSID;
	if (!str_parse.good()) { std::cerr << "invalid SSID\n"; return 1; }
	
	str_parse >> dataR->addr2;
	if (!str_parse.good()) { std::cerr << "invalid addr2\n"; return 1; }
	str_parse >> date;
	if (!str_parse.good()) { std::cerr << "invalid date\n"; return 1; }
	str_parse >> hour;
	//if (!str_parse.good()) { std::cerr << "invalid hour\n"; return 1; }

	//str_parse >> dataR.timestamp;


	
	char* dbdatechar = new char[11];
	char *datechar = (char *)date.c_str();
	int i;
	for (i = 0; i <= 3; i++) dbdatechar[i] = datechar[i + 6];
	dbdatechar[i] = '-';
	for (i = 5; i <= 6; i++) dbdatechar[i] = datechar[i - 2];
	dbdatechar[i] = '-';
	for (i = 8; i <= 9; i++) dbdatechar[i] = datechar[i - 8];
	dbdatechar[10]='\0';
	string dbdate(dbdatechar);
	


	dataR->timestamp = dbdate + " " + hour;
	//cout << "dataR.ID: " << dataR.ID << endl;
	//cout << "dataR.hash: " << dataR.hash << endl;
	cout << "dataR.ID: " << dataR->ID << endl;
	cout << "dataR.hash: " << dataR->hash << endl;
	cout << "dataR.channel: " << dataR->channel << endl;
	cout << "dataR.rssi: " << dataR->rssi << endl;
	cout << "dataR.seqNumber: " << dataR->seqNumber << endl;
	cout << "dataR.SSID: " << dataR->SSID << endl;
	cout << "dataR.addr2: " << dataR->addr2 << endl;
	cout << "dataR.timestamp: " << dataR->timestamp << endl;

	return 0;

}

void connectToDb() {
	
	
}

int send_timestamp(SOCKET ClientSocket)
{
	int iSendResult, iResult;
	time_t timestamp = time(nullptr);
	struct tm tm;
	localtime_s(&tm, &timestamp);
	char str[100];
	char charTime[100];
	asctime_s(str, sizeof str, &tm);
	string strTime = to_string(timestamp);
	strcpy_s(charTime, sizeof(charTime), strTime.c_str());


	cout << "TIME: " << str
		<< timestamp << " = " << charTime << " seconds since the Epoch\n";

	printf("\ninvio TIMESTAMP..\n");
	iSendResult = send(ClientSocket, charTime, strlen(charTime), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}


}


unsigned __stdcall ClientSession(void *data)
{
	WSADATA wsaData;
	int iResult;

	printf("nuovo thread creato!\n");
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	time_t timestamp;
	data_received dataR;
	char nbytes_char[10];
	int nbytes;
	int bytesTOT;
	int bytes_left;
	int allocSize;
	int usedSize;
	int sessionID;//salvo il numero della shedina
	int res;
	int timer = 5;
	//mutex m;
	// Receive until the peer shuts down the connection


	//connectToDb();

	//connect to DB
	MYSQL *connect = mysql_init(NULL);
	connect = mysql_real_connect(connect, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0);
	if (!connect) { printf("MySQL Initialization or Connection Failed!\n"); return 1; }



	SOCKET ClientSocket = (SOCKET)data;

	//INVIA TIMESTAMP 

	timestamp = time(nullptr);
	struct tm tm;
	localtime_s(&tm, &timestamp);
	char str[100];
	char charTime[100];
	asctime_s(str, sizeof str, &tm);
	string strTime = to_string(timestamp);
	strcpy_s(charTime, sizeof(charTime), strTime.c_str());


	cout << "TIME: " << str
		<< timestamp << " = " << charTime << " seconds since the Epoch\n";

	printf("Attendo sulla recv\n");
	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		printf("Bytes received: %d\n", iResult);
		printf("MAC : ");
		for (int i = 0; i < iResult; i++) {
			putchar(recvbuf[i]);
		}
		string mac_recv(recvbuf);

		mac_recv.erase(mac_recv.begin() + iResult, mac_recv.end());

		if (MacID.find(mac_recv) != MacID.end()) {
			lock_guard<mutex> l(m);
			sessionID = MacID[mac_recv];
			//std::cout << "map contains this MAC " << mac_recv << " already!\n";
			std::cout << " and map contains this MAC.\n";
		}
		else {
			//lock_guard<mutex> l(m);
			//MacID[mac_recv] = ID;
			//dataR.ID = ID;
			//ID++;
			printf("Schedina non presente in db");
			return 1;
		}


		/*printf("\ninvio TIMESTAMP..\n");
		iSendResult = send(ClientSocket, charTime, strlen(charTime), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

		printf("Bytes sent: %d\n", iSendResult);*/
		res = send_timestamp(ClientSocket);
		if (res == 1) return 1;


	}
	else if (iResult == 0)
		printf("Connection closing...\n");
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		return 1;
	}

	while (1) {
		iResult = 0;

		printf("Attendo sulla recv\n");
		iResult = recv(ClientSocket, nbytes_char, sizeof(nbytes_char), 0); //ricevo il numero totale di byte da ricevere
		if (iResult > 0) {
			printf("char con il num di byte ricevuta: %s\n", nbytes_char);
			nbytes = atoi(nbytes_char);
			printf("Bytes to be received: %d\n", nbytes);
			//printf("iResult di nbytes= %d\n", iResult);
			/*for (int i = 0; i < iResult; i++) {
			putchar(recvbuf[i]);
			}*/

		}
		bytes_left = nbytes;
		vector<char> recvbuf(DEFAULT_BUFLEN);
		/*char* recvbuf_malloc;
		recvbuf_malloc = (char)malloc(nbytes * sizeof(char)+1);*/


		while (bytes_left > 0) {

			iResult = recv(ClientSocket, &recvbuf[0], nbytes, 0);
			printf("iResult= %d\n", iResult);
			bytes_left -= iResult;

			printf("bytes_left= %d\n", bytes_left);

			/*std::vector<char>::iterator it;
			for (it = recvbuf.begin(); it != recvbuf.end(); ++it) {
			std::cout << *it;
			}*/

		}



		string recv_s(recvbuf.begin(), recvbuf.end());
		recv_s.erase(recv_s.begin() + nbytes, recv_s.end());
		cout << "Schedina " << sessionID <<"  : " << recv_s << endl;
		
		
		if (recv_s == "FINE")
		{
			//avviso il padre della fine del minuto
			lock_guard<mutex> lg(m);
			fine_ricezione++;
			cv.notify_one();

			if (timer == 0)
			{
				res = send_timestamp(ClientSocket);
				if (res == 1) return 1;
				timer = 5;
			}
			else
				timer--;
		}

		//else
			//bool result = readLine(&dataR, recv_s);

		
		bool result;

		result = readLine(&dataR, recv_s);
		string myquery = "INSERT INTO segnali_rilevati(ID_SCHEDA, HASH_P, SSID, MARCA_TEMPORALE, LIVELLO_SEGNALE, MAC, NUM_SEQUENZA) VALUES('"
			+ to_string(sessionID)		+ "', '"
			+ dataR.hash				+ "', '"
			+ dataR.SSID				+ "', '"
			+ dataR.timestamp			+ "', '"
			+ to_string(dataR.rssi)		+ "', '"
			+ dataR.addr2				+ "', '"
			+ to_string(dataR.seqNumber) +"')";
		
		
		//int retval=mysql_query(connect, myquery.c_str());
		int retval = mysql_real_query(connect, myquery.c_str(), myquery.length());

		cout << myquery.c_str() << endl;
		cout << "Query: " << retval<<endl;
		

	}


	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	return 0;
}

int __cdecl main(void)
{
	MYSQL *connect = mysql_init(NULL);
	connect = mysql_real_connect(connect, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0);
	if (!connect) { printf("MySQL Initialization or Connection Failed!\n"); return 1; }

	mysql_query(connect, "select ID, MAC from schedine");
	MYSQL_RES *res_set; MYSQL_ROW row;

	res_set = mysql_store_result(connect);  //unsigned int numrows = mysql_num_rows(res_set);   
	int num_schede = res_set->row_count;
	cout << num_schede << "schede collegate:" << endl;
	while (row = mysql_fetch_row(res_set)) {
		printf("  %s %s\n", row[0], row[1]); //Print the row data
		MacID[row[1]] = stoi(row[0]); //string to int
	}
	
	


	
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;



	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}


	// Setup the TCP listening socket
	iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Bind success\n");

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Listen success\n");
	// Accept a client socket
	//while (1)
	
	for(int nfigli = 0; nfigli<num_schede; nfigli++)
	{
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);

			return 1;
		}
		printf("Creo un nuovo thread\n");
		unsigned threadID;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)ClientSocket, 0, &threadID);
		printf("accept done!\n");
		// No longer need server socket
		//closesocket(ClientSocket);

	};

	printf("TUTTE LE SCHEDINE SI SONO COLLEGATE\n");


	while (1) {

		//controllo se tutti i figli hanno ricevuto
		unique_lock<mutex> ul(m);//controlla se è unique!!
		cv.wait(ul, [&]() {
			if (fine_ricezione == num_schede) return true; //svegliati
		});
	}

	WSACleanup();
	return 0;
}