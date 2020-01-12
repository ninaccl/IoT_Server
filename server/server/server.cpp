﻿#include "server.h"
#include "sclient.h"

//全局变量
CRITICAL_SECTION cs;	//保护数据的临界区对象
BOOL bConning;	//与客户端的连接状态
BOOL clientConn;	//连接客户端标记
SOCKET listenSocket;	//服务器监听套接字
HANDLE hAcceptThread;	//接受客户端连接线程句柄
HANDLE hCleanThread;	//数据接收线程
ClIENTVECTOR clientvector;	//存储子套接字
BOOL bSend;	//发送标记位
char dataBuf[MAX_NUM_BUF];	//写缓冲区

//初始化
BOOL initServer(void)
{
	//初始化全局变量
	initMember();

	//初始化SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

//初始化全局变量
void initMember(void)
{
	InitializeCriticalSection(&cs);	//初始化临界区
	bConning = FALSE;	//初始化服务器为没有运行状态
	clientConn = FALSE;	//初始化客户端连接标记
	listenSocket = INVALID_SOCKET;	//设置为无效的套接字
	hAcceptThread = NULL;	//设置为NULL
	hCleanThread = NULL;
	clientvector.clear();
	bSend = FALSE;	//初始化发送标志位
	memset(dataBuf, 0, MAX_NUM_BUF);	//初始化写缓冲区
}

//初始化SOCKET
bool initSocket(void)
{
	//返回值
	int reVal;

	//初始化Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);

	//创建套接字
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		//WSACleanup();
		return FALSE;
	}

	//绑定套接字
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(listenSocket, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//监听
	reVal = listen(listenSocket, CONN_NUM);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

//启动服务
bool startService(void)
{
	BOOL reVal = TRUE;	//返回值

	showTipMsg(START_SERVER);	//提示用户输入
	char cInput;
	do
	{
		cin >> cInput;
		if (cInput == 's' || cInput == 'S')
		{
			if (createCleanAndAcceptThread())
			{
				showServerStartMsg(TRUE);
			}
			else
			{
				reVal = FALSE;
			}
			break;
		}
		else
		{
			showTipMsg(START_SERVER);
		}
	} while (cInput != 's'&&cInput != 'S');

	cin.sync();	//清空输入缓冲区
	return reVal;
}


//接受客户端连接线程
BOOL createCleanAndAcceptThread(void)
{
	bConning = TRUE;//设置服务器为运行状态

	//创建释放资源线程
	unsigned long ulThreadId;
	hCleanThread = CreateThread(NULL, 0, cleanThread, NULL, 0, &ulThreadId);
	if (NULL == hCleanThread)
	{
		return FALSE;
	}
	else
	{
		CloseHandle(hCleanThread);
	}

	//创建接收客户端请求线程
	hAcceptThread = CreateThread(NULL, 0, acceptThread, NULL, 0, &ulThreadId);
	if (NULL == hAcceptThread)
	{
		bConning = FALSE;
		return FALSE;
	}
	else
	{
		CloseHandle(hAcceptThread);
	}
	return bConning;
}

//接受客户端连接
DWORD __stdcall acceptThread(void* pParam)
{
	SOCKET  acceptSocket;	//接受客户端连接的套接字
	sockaddr_in addrClient;	//客户端SOCKET地址
	while (bConning)	//判断服务器状态
	{
		int	lenClient = sizeof(sockaddr_in);	//地址长度
		memset(&addrClient, 0, lenClient);	//初始化
		acceptSocket = accept(listenSocket, (sockaddr*)&addrClient, &lenClient); //接受客户请求
		if (acceptSocket == INVALID_SOCKET)
		{
			int nErrCode = GetLastError();
			if (nErrCode == WSAEWOULDBLOCK)	//无法立即完成一个非阻挡性套接字操作
			{
				Sleep(TIMEFOR_ACCEPTTHREAD_SLEEP);
				continue;	//继续等待
			}
			else
			{
				return 0;	//线程退出
			}
		}
		else//接受客户端的请求
		{
			clientConn = TRUE;	//客户端已经连接上
			CClient *pClient = new CClient(acceptSocket, addrClient);
			EnterCriticalSection(&cs);
			//显示客户端的ip和端口
			char *pClientIP = inet_ntoa(addrClient.sin_addr);
			u_short clientPort = ntohs(addrClient.sin_port);
			cout << "Accept a client IP: " << pClientIP << "\tPort: " << clientPort << endl;
			clientvector.push_back(pClient);
			LeaveCriticalSection(&cs);

			pClient->StartRuning();
		}
	}
	return 0;//线程退出
}

//清理资源线程
DWORD __stdcall cleanThread(void* pParam)
{
	while (bConning)
	{
		EnterCriticalSection(&cs);	//进入临界区

		//若没有已连接客户端
		if (clientvector.size() == 0)
		{
			clientConn = FALSE;
		}
		else
		{
			//清理已断开的连接客户端内存空间
			ClIENTVECTOR::iterator iter = clientvector.begin();
			for (iter;iter != clientvector.end();)
			{
				CClient *pClient = (CClient*)*iter;
				if (!pClient->IsConning())	//客户端线程已经退出
				{
					//显示断联客户端的ip和端口
					char *pClientIP = inet_ntoa(pClient->GetAddr().sin_addr);
					u_short clientPort = ntohs(pClient->GetAddr().sin_port);
					cout << "Leave a client IP: " << pClientIP << "\tPort: " << clientPort << endl;

					iter = clientvector.erase(iter);	//删除节点
					delete pClient;
					pClient = NULL;
				}
				else
				{
					iter++;
				}
			}
		}

		LeaveCriticalSection(&cs);	//离开临界区

		Sleep(TIMEFOR_CLEANTHREAD_SLEEP);
	}

	//服务器停止工作
	if (!bConning)
	{
		//断开每个连接，线程退出
		EnterCriticalSection(&cs);
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//如果客户端的连接还存在，则断开连接，线程退出
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			iter++;
		}
		//离开临界区
		LeaveCriticalSection(&cs);

		//给连接客户端线程时间，使其自动退出
		Sleep(TIMEFOR_CLEANTHREAD_SLEEP);
	}

	clientvector.clear();	//清空链表
	clientConn = FALSE;

	return 0;
}

//释放资源
void  exitServer(void)
{
	closesocket(listenSocket);	//关闭SOCKET
	WSACleanup();	//卸载Windows Sockets DLL
}

void showTipMsg(int input)
{
	EnterCriticalSection(&cs);
	if (START_SERVER == input)          //启动服务器
	{
		cout << "**********************" << endl;
		cout << "* s(S): Start server *" << endl;
		cout << "* e(E): Exit  server *" << endl;
		cout << "**********************" << endl;
		cout << "Please input:";

	}
	else if (INPUT_DATA == input)
	{
		cout << "*******************************************" << endl;
		cout << "* please connect clients,then send data   *" << endl;
		cout << "* write+num+data:Send data to client-num  *" << endl;
		cout << "*   all+data:Send data to all clients     *" << endl;
		cout << "*          e(E): Exit  server             *" << endl;
		cout << "*******************************************" << endl;
		cout << "Please input:" << endl;
	}
	LeaveCriticalSection(&cs);
}

//显示启动服务器成功与失败消息
void  showServerStartMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "**********************" << endl;
		cout << "* Server succeeded!  *" << endl;
		cout << "**********************" << endl;
	}
	else {
		cout << "**********************" << endl;
		cout << "* Server failed   !  *" << endl;
		cout << "**********************" << endl;
	}

}

//显示服务器退出消息
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}


//等待键盘输入处理数据
void inputAndOutput(void)
{
	char sendBuf[MAX_NUM_BUF];

	showTipMsg(INPUT_DATA);

	while (bConning)
	{
		memset(sendBuf, 0, MAX_NUM_BUF);	//清空接收缓冲区
		if (_kbhit())//检测键盘有无按键，有按键_kbhit()返回一个非零值
		{
			cin.getline(sendBuf, MAX_NUM_BUF);	//输入数据
			//发送数据
			handleData(sendBuf);
		}
	}
}


//依据输入选择不同模式处理数据
void handleData(char* str)
{
	CClient *sClient;
	string recvsting;
	char cnum;	//定义发给第几个连接的服务器，从0开始
	int num;	//定义服务器编号，从1开始

	if (((*str) != 0))
	{
		//加上ODOA回车换行
		*(str + strlen(str)) = '\r';
		*(str + strlen(str)) = '\n';
		*(str + strlen(str)) = 0;

		if (!strncmp(WRITE, str, strlen(WRITE))) //判断输入指令是否为
		{
			EnterCriticalSection(&cs);
			str += strlen(WRITE);
			cnum = *str++;
			num = cnum - '1';
			//增加容量处理
			if (num<clientvector.size())
			{
				sClient = clientvector.at(num);     //发送到指定客户端
				strcpy(dataBuf, str);
				/*dataBuf[strlen(str)] = '\r';
				dataBuf[strlen(str) + 1] = '\n';
				dataBuf[strlen(str) + 2] = 0;*/
				sClient->IsSend();
				LeaveCriticalSection(&cs);
			}
			else                                    //不在范围
			{
				cout << "The client isn't in scope!" << endl;
				LeaveCriticalSection(&cs);
			}

		}
		else if (!strncmp(WRITE_ALL, str, strlen(WRITE_ALL)))
		{
			EnterCriticalSection(&cs);
			str += strlen(WRITE_ALL);
			strcpy(dataBuf, str);
			bSend = TRUE;
			LeaveCriticalSection(&cs);
		}
		else if ('e' == str[0] || 'E' == str[0])     //判断是否退出
		{
			bConning = FALSE;
			showServerExitMsg();
			Sleep(TIMEFOR_THREAD_EXIT);
			exitServer();
		}
		else
		{
			cout << "Input error!!" << endl;
		}

	}
}