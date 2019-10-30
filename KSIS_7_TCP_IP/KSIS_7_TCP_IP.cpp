#include "pch.h"
#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
using namespace std;

//typedef struct {
//	unsigned char Ttl;		//время доставки
//	unsigned char Tos;   	//тип сервиса
//	unsigned char Flags;	//флаги IP заголовка
//	unsigned char OptionsSize;
//	//размер опций в байтах 
//	unsigned char *OptionsData;
//	//указатель на опции
//} IP_OPTION_INFORMATION, *PMP_OPTION_INFORMATION;

void Ping(const char* cHost, unsigned int Timeout = 60, unsigned int RequestCount = 10)
{
	// Создать файл сервиса
	HANDLE hIP = IcmpCreateFile(); //создает соединение, с которым мы собираемся работать.
	if (hIP == INVALID_HANDLE_VALUE)
	{
		WSACleanup();
		return;
	}
	char SendData[36] = "Data for ping";						//буфер для передачи
	int LostPacketsCount = 0;									// кол-во потерянных пакетов
	unsigned int MaxMS = 0;										// максимальное время ответа (мс)
	int MinMS = -1;											    // минимальное время ответа (мс)

	PICMP_ECHO_REPLY pIpe =										// Выделяем память под пакет – буфер ответа
		(PICMP_ECHO_REPLY)GlobalAlloc(GHND,
			sizeof(ICMP_ECHO_REPLY)
			+ sizeof(SendData));
	if (pIpe == 0) {
		IcmpCloseHandle(hIP);
		WSACleanup();
		return;
	}
	pIpe->Data = SendData;
	pIpe->DataSize = sizeof(SendData);
	unsigned long ipaddr = inet_addr(cHost);
	IP_OPTION_INFORMATION option = { 100, 0, 0, 0, 0 };        //опции для включения в заголовок IP пакета

	for (unsigned int c = 0; c < RequestCount; c++)
	{
		int dwStatus = IcmpSendEcho(hIP,
			ipaddr,
			SendData,
			sizeof(SendData),
			&option,
			pIpe,
			sizeof(ICMP_ECHO_REPLY) +
			sizeof(SendData),
			Timeout);
		if (dwStatus > 0)
		{
			for (int i = 0; i < dwStatus; i++)
			{
				unsigned char* pIpPtr =
					(unsigned char*)&pIpe->Address;
				cout << "Ответ от  " << (int)*(pIpPtr)
					<< "." << (int)*(pIpPtr + 1)
					<< "." << (int)*(pIpPtr + 2)
					<< "." << (int) *(pIpPtr + 3)
					<< ": число байт = " << pIpe->DataSize
					<< " время = " << pIpe->RoundTripTime
					<< "мс TTL = " << (int)pIpe->Options.Ttl;
				MaxMS = (MaxMS > pIpe->RoundTripTime) ?
					MaxMS : pIpe->RoundTripTime;
				MinMS = (MinMS < (int)pIpe->RoundTripTime
					&& MinMS >= 0) ?
					MinMS : pIpe->RoundTripTime;
				cout << endl;
			}
		}
		else
		{
			if (pIpe->Status)
			{
				LostPacketsCount++;
				switch (pIpe->Status)
				{
				case IP_DEST_NET_UNREACHABLE:
				case IP_DEST_HOST_UNREACHABLE:
				case IP_DEST_PROT_UNREACHABLE:
				case IP_DEST_PORT_UNREACHABLE:
					cout << "Remote host may be down." << endl;
					break;
				case IP_REQ_TIMED_OUT:
					cout << "Request timed out." << endl;
					break;
				case IP_TTL_EXPIRED_TRANSIT:
					cout << "TTL expired in transit." << endl;
					break;
				default:
					cout << "Error code: %ld"
						<< pIpe->Status << endl;
					break;
				}
			}
		}
	}
	IcmpCloseHandle(hIP); //закрывает соединение
	WSACleanup();
	if (MinMS < 0) MinMS = 0;
	unsigned char* pByte = (unsigned char*)&pIpe->Address;
	cout << "Статистика Ping    "
		<< (int)*(pByte)
		<< "." << (int)*(pByte + 1)
		<< "." << (int)*(pByte + 2)
		<< "." << (int)*(pByte + 3) << endl;
	cout << "\tПакетов: отправлено = " << RequestCount
		<< ", поучено = "
		<< RequestCount - LostPacketsCount
		<< ", потеряно = " << LostPacketsCount
		<< "<" << (int)(100 / (float)RequestCount)*
		LostPacketsCount
		<< " % потерь>, " << endl;
	cout << "Приблизительное время приема-передачи:"
		<< endl << "Минимальное = " << MinMS
		<< "мс, Максимальное = " << MaxMS
		<< "мс, Среднее = " << (MaxMS + MinMS) / 2
		<< "мс" << endl;

}

#define PARM_N "-n:"
#define PARM_W "-w:"


int main(int argc, char * argv[])
{
	setlocale(LC_ALL, "RUS");
	char  n[30];
	char  w[30];
	unsigned int k = 10, y = 60;
	for (int i = 1; i < argc; i++)
	{
		if (strstr(argv[i], PARM_N))
		{
			strcpy_s(n, argv[i] + strlen(PARM_N));
			k = atoi(n);
		}
		else
		if (strstr(argv[i], PARM_W))
		{
			strcpy_s(w, argv[i] + strlen(PARM_W));
			y = atoi(w);
		}
	}
	if (argv[1] != nullptr)
		Ping(argv[1], y, k);


	system("pause");
	return 0;

}