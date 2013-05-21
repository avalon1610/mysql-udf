// bin2c.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

// no cmd window
//#ifdef UNICODE
//#pragma comment(linker, "/subsystem:\"windows\"  /entry:\"wmainCRTStartup\"")
//#else
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
//#endif

#pragma warning(disable:4996)


int _tmain(int argc, _TCHAR* argv[])
{
	int nRet = -1;
	// the makefil0 of nt mbr takes 6 paramter, so compatible it
	if (argc == 1)
	{
		// print usage
		printf("usage:\n");
		printf("bin2c SrcFile DstSize SrcBegin SrcEnd DstFile ArrayName [HexMod]");
		return -1;
	}
	if (argc < 4) 
	{
		printf("incorrect parameter count!");
		return -1;
	}

	CHAR szSrcFile[MAX_PATH] = {0};         // source file full path
	DWORD dwDstSize = 0;                            // .h file size
	DWORD dwSrcBegin = 0;                           // source file begin position
	DWORD dwSrcEnd = 0;                                     // source file end position
	CHAR szDstFile[MAX_PATH] = {0};         // .h file full path
	CHAR szArrayName[MAX_PATH] = {0};       // unsigned char array name
	BOOL bHex = FALSE;                                      // hex mode ? default decimal

	// init parameter from command line
	// check parameter valid at used time
	strcpy_s(szSrcFile, MAX_PATH, argv[1]);
	//dwDstSize = _ttoi(argv[2]);
	//dwSrcBegin = _ttoi(argv[3]);
	//dwSrcEnd = _ttoi(argv[4]);
	strcpy_s(szDstFile, MAX_PATH, argv[2]);
	strcpy_s(szArrayName, MAX_PATH, argv[3]);
	/*
	if (argc > 7)
	{
		bHex = _ttoi(argv[7]);
	}
	*/
	bHex = TRUE;

	HANDLE hSrcFile = ::CreateFile(szSrcFile, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hSrcFile == INVALID_HANDLE_VALUE)
	{
		printf("can not open source file(%d)", ::GetLastError());
		return -1;
	}

	HANDLE hDstFile = ::CreateFile(szDstFile,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDstFile == INVALID_HANDLE_VALUE)
	{
		printf("can not create .h file(%d)", ::GetLastError());
		CloseHandle(hSrcFile);
		return -1;
	}

	DWORD dwSrcFileSize = ::GetFileSize(hSrcFile, NULL);
	if (dwSrcFileSize == INVALID_FILE_SIZE)
	{
		printf("get source file Size failed(%d)", ::GetLastError());
		goto Exit;
	}

	dwSrcEnd = dwSrcFileSize;

	/*
	if ((dwDstSize > dwSrcFileSize) ||
		(dwSrcBegin > dwSrcFileSize) ||
		(dwSrcEnd > dwSrcFileSize) ||
		(dwSrcBegin > dwSrcEnd) ||
		((dwSrcEnd - dwSrcBegin) > dwDstSize))
	{
		printf("incorrect file size parameter!");
		goto Exit;
	}
	*/


	// .h is ANSI file
	CHAR szDstFileDefine[MAX_PATH] = {0};
	CHAR* pLastSlash = strrchr(szDstFile, '\\');
	if (pLastSlash != NULL)
	{
		strcpy_s(szDstFileDefine, MAX_PATH, pLastSlash + 1);
		CHAR* pDot = strrchr(szDstFileDefine, '.');
		if (pDot)
		{
			*pDot = '_';
		}
	}
	else
	{
		strcpy_s(szDstFileDefine, MAX_PATH, szDstFile);
		CHAR* pDot = strrchr(szDstFileDefine, '.');
		if (pDot)
		{
			*pDot = '_';
		}
	}

	//CHAR* szFmtTitle = "#ifndef %s\r\n#define %s\r\n\r\n\r\n\r\nunsigned char %s[] = {";
	CHAR* szFmtTitle = "\r\n\r\nunsigned char *%s = \"";
	//CHAR* szFmtEnd = "\r\n};\r\n\r\n#endif // %s";
	CHAR* szFmtEnd = "\";";

	CHAR szTitle[2 * MAX_PATH] = {0};
	//sprintf(szTitle, szFmtTitle, strupr(szDstFileDefine), strupr(szDstFileDefine), szArrayName);
	sprintf(szTitle, szFmtTitle, szArrayName);
	
	CHAR szEnd[2 * MAX_PATH] = {0};
	//sprintf(szEnd, szFmtEnd, strupr(szDstFileDefine));
	sprintf(szEnd, szFmtEnd);

	DWORD dwWrited = 0;
	if (!::WriteFile(hDstFile, (LPVOID)szTitle, strlen(szTitle), &dwWrited, NULL))
	{
		printf("write file title failed(5d)", ::GetLastError());
		goto Exit;
	}


	::SetFilePointer(hSrcFile, dwSrcBegin, NULL, FILE_BEGIN);
	UCHAR szRead = '0';
	CHAR* szWrite = NULL;
	if (bHex)
	{
		szWrite = new CHAR[7];
	} 
	else
	{
		szWrite = new CHAR[6];
	}
	DWORD dwReaded = 0;
	for (DWORD i = dwSrcBegin; i < dwSrcEnd; i++)
	{
		if (((i - dwSrcBegin) % 32) == 0)
		{
			static const CHAR szLine[] = "\"\r\n\"";
			::WriteFile(hDstFile, szLine, strlen(szLine), &dwWrited, NULL);
		}

		::ReadFile(hSrcFile, (LPVOID)&szRead, sizeof(UCHAR), &dwReaded, NULL) && (dwReaded != 0);		

		if (bHex)
			// sprintf(szWrite, "0x%02X, ", szRead); // 7 byte
			sprintf(szWrite,"%02X",szRead);
		else
			sprintf(szWrite, "%3u, ", szRead);      // 6 byte

		::WriteFile(hDstFile, (LPVOID)szWrite, strlen(szWrite), &dwWrited, NULL);
	}

	//::SetFilePointer(hDstFile, -2, NULL, FILE_CURRENT);     // remove last ','

	if (szWrite)
	{
		delete []szWrite;
	}


	if (!::WriteFile(hDstFile, szEnd, strlen(szEnd), &dwWrited, NULL))
	{
		printf("write file end failed(%d)", ::GetLastError());
		goto Exit;
	}
	

	printf("success!");
	nRet = 0;
Exit:
	if (hSrcFile)
	{
		CloseHandle(hSrcFile);
	}
	if (hDstFile)
	{
		CloseHandle(hDstFile);
	}
	return nRet;
}