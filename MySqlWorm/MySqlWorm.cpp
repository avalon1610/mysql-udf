// MySqlWorm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mysql.h"


int _tmain(int argc, _TCHAR* argv[])
{
	MYSQL *mysql = mysql_init(NULL);
	mysql_options(mysql,MYSQL_READ_DEFAULT_GROUP,"libmysqld_client");
	mysql_close();
	mysql_library_end();
	return 0;
}

