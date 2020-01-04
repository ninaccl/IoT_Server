#include "server.h"
#include "sclient.h"

//ȫ�ֱ���
CRITICAL_SECTION cs;	//�������ݵ��ٽ�������
BOOL bConning;	//��ͻ��˵�����״̬
BOOL clientConn;	//���ӿͻ��˱��
SOCKET listenSocket;	//�����������׽���
HANDLE hAcceptThread;	//���ܿͻ��������߳̾��
HANDLE hCleanThread;	//���ݽ����߳�
ClIENTVECTOR clientvector;	//�洢���׽���
BOOL bSend;	//���ͱ��λ
char dataBuf[MAX_NUM_BUF];	//д������

//��ʼ��
BOOL initServer(void)
{
	//��ʼ��ȫ�ֱ���
	initMember();

	//��ʼ��SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

//��ʼ��ȫ�ֱ���
void initMember(void)
{
	InitializeCriticalSection(&cs);	//��ʼ���ٽ���
	bConning = FALSE;	//��ʼ��������Ϊû������״̬
	clientConn = FALSE;	//��ʼ���ͻ������ӱ��
	listenSocket = INVALID_SOCKET;	//����Ϊ��Ч���׽���
	hAcceptThread = NULL;	//����ΪNULL
	hCleanThread = NULL;
	clientvector.clear();
	bSend = FALSE;	//��ʼ�����ͱ�־λ
	memset(dataBuf, 0, MAX_NUM_BUF);	//��ʼ��д������
}

//��ʼ��SOCKET
bool initSocket(void)
{
	//����ֵ
	int reVal;

	//��ʼ��Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);

	//�����׽���
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		//WSACleanup();
		return FALSE;
	}

	//���׽���
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(listenSocket, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//����
	reVal = listen(listenSocket, CONN_NUM);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

//��������
bool startService(void)
{
	BOOL reVal = TRUE;	//����ֵ

	showTipMsg(START_SERVER);	//��ʾ�û�����
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

	cin.sync();	//������뻺����
	return reVal;
}


//���ܿͻ��������߳�
BOOL createCleanAndAcceptThread(void)
{
	bConning = TRUE;//���÷�����Ϊ����״̬

	//�����ͷ���Դ�߳�
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

	//�������տͻ��������߳�
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

//���ܿͻ�������
DWORD __stdcall acceptThread(void* pParam)
{
	SOCKET  acceptSocket;	//���ܿͻ������ӵ��׽���
	sockaddr_in addrClient;	//�ͻ���SOCKET��ַ
	while (bConning)	//�жϷ�����״̬
	{
		int	lenClient = sizeof(sockaddr_in);	//��ַ����
		memset(&addrClient, 0, lenClient);	//��ʼ��
		acceptSocket = accept(listenSocket, (sockaddr*)&addrClient, &lenClient); //���ܿͻ�����
		if (acceptSocket == INVALID_SOCKET)
		{
			int nErrCode = GetLastError();
			if (nErrCode == WSAEWOULDBLOCK)	//�޷��������һ�����赲���׽��ֲ���
			{
				Sleep(TIMEFOR_ACCEPTTHREAD_SLEEP);
				continue;	//�����ȴ�
			}
			else
			{
				return 0;	//�߳��˳�
			}
		}
		else//���ܿͻ��˵�����
		{
			clientConn = TRUE;	//�ͻ����Ѿ�������
			CClient *pClient = new CClient(acceptSocket, addrClient);
			EnterCriticalSection(&cs);
			//��ʾ�ͻ��˵�ip�Ͷ˿�
			char *pClientIP = inet_ntoa(addrClient.sin_addr);
			u_short clientPort = ntohs(addrClient.sin_port);
			cout << "Accept a client IP: " << pClientIP << "\tPort: " << clientPort << endl;
			clientvector.push_back(pClient);
			LeaveCriticalSection(&cs);

			pClient->StartRuning();
		}
	}
	return 0;//�߳��˳�
}

//������Դ�߳�
DWORD __stdcall cleanThread(void* pParam)
{
	while (bConning)
	{
		EnterCriticalSection(&cs);	//�����ٽ���

		//��û�������ӿͻ���
		if (clientvector.size() == 0)
		{
			clientConn = FALSE;
		}
		else
		{
			//�����ѶϿ������ӿͻ����ڴ�ռ�
			ClIENTVECTOR::iterator iter = clientvector.begin();
			for (iter;iter != clientvector.end();)
			{
				CClient *pClient = (CClient*)*iter;
				if (!pClient->IsConning())	//�ͻ����߳��Ѿ��˳�
				{
					iter = clientvector.erase(iter);	//ɾ���ڵ�
					delete pClient;
					pClient = NULL;
				}
				else
				{
					iter++;
				}
			}
		}

		LeaveCriticalSection(&cs);	//�뿪�ٽ���

		Sleep(TIMEFOR_CLEANTHREAD_SLEEP);
	}

	//������ֹͣ����
	if (!bConning)
	{
		//�Ͽ�ÿ�����ӣ��߳��˳�
		EnterCriticalSection(&cs);
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//����ͻ��˵����ӻ����ڣ���Ͽ����ӣ��߳��˳�
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			iter++;
		}
		//�뿪�ٽ���
		LeaveCriticalSection(&cs);

		//�����ӿͻ����߳�ʱ�䣬ʹ���Զ��˳�
		Sleep(TIMEFOR_CLEANTHREAD_SLEEP);
	}

	clientvector.clear();	//�������
	clientConn = FALSE;

	return 0;
}

//�ͷ���Դ
void  exitServer(void)
{
	closesocket(listenSocket);	//�ر�SOCKET
	WSACleanup();	//ж��Windows Sockets DLL
}

void showTipMsg(int input)
{
	EnterCriticalSection(&cs);
	if (START_SERVER == input)          //����������
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

//��ʾ�����������ɹ���ʧ����Ϣ
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

//��ʾ�������˳���Ϣ
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}


//�ȴ��������봦������
void inputAndOutput(void)
{
	char sendBuf[MAX_NUM_BUF];

	showTipMsg(INPUT_DATA);

	while (bConning)
	{
		memset(sendBuf, 0, MAX_NUM_BUF);	//��ս��ջ�����
		if (_kbhit())//���������ް������а���_kbhit()����һ������ֵ
		{
			cin.getline(sendBuf, MAX_NUM_BUF);	//��������
			//��������
			handleData(sendBuf);
		}
	}
}


//��������ѡ��ͬģʽ��������
void handleData(char* str)
{
	CClient *sClient;
	string recvsting;
	char cnum;	//���巢���ڼ������ӵķ���������0��ʼ
	int num;	//�����������ţ���1��ʼ

	if (((*str) != 0))
	{
		if (!strncmp(WRITE, str, strlen(WRITE))) //�ж�����ָ���Ƿ�Ϊ
		{
			EnterCriticalSection(&cs);
			str += strlen(WRITE);
			cnum = *str++;
			num = cnum - '1';
			//������������
			if (num<clientvector.size())
			{
				sClient = clientvector.at(num);     //���͵�ָ���ͻ���
				strcpy(dataBuf, str);
				dataBuf[strlen(str)] = '\r';
				dataBuf[strlen(str) + 1] = '\n';
				dataBuf[strlen(str) + 2] = 0;
				sClient->IsSend();
				LeaveCriticalSection(&cs);
			}
			else                                    //���ڷ�Χ
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
		else if ('e' == str[0] || 'E' == str[0])     //�ж��Ƿ��˳�
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