#pragma once
#include <iostream>
#include <winsock2.h>
#include <winbase.h>
#include <vector>
#include <map>
#include <string>
#include <process.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "sclient.h"

// Need to link with Ws2_32.lib
using namespace std;
#pragma comment(lib, "ws2_32.lib")			//��̬�⺯��


#define SERVER_SETUP_FAIL 1	//����������ʧ��
#define START_SERVER 1	//��ʾ��ʼ������ʾ
#define INPUT_DATA 2	//��ʾ����ʲô����

#define SERVERPORT 8081	//������TCP�˿�
#define CONN_NUM 10	//���ӿͻ�������
#define TIMEFOR_ACCEPTTHREAD_SLEEP 500	//�ȴ��ͻ��������߳�˯��ʱ��
#define TIMEFOR_CLEANTHREAD_SLEEP 1500	//������Դ�߳��˳�ʱ��
#define TIMEFOR_THREAD_EXIT 5000	//���߳�˯��ʱ��
#define WRITE_ALL "all"	//���������ݷ�������
#define WRITE "write"	 //���ͱ�־
#define READ "read"	//������ʾ��־
#define READ_ALL "read all"	//�������пͻ�������
typedef vector<CClient*> ClIENTVECTOR;		//��������
typedef vector<string> SVECTOR;             //�����ַ�

//ȫ�ֱ���
extern char	dataBuf[MAX_NUM_BUF];	//д������
extern BOOL bConning;	//��ͻ��˵�����״̬
extern BOOL bSend;	//���ͱ��λ
extern BOOL clientConn;	//���ӿͻ��˱��
extern SOCKET listenSocket;	//�����������׽���
extern CRITICAL_SECTION cs;	//�������ݵ��ٽ�������
extern HANDLE hAcceptThread;	//���ܿͻ��������߳̾��
extern HANDLE hCleanThread;	//���ݽ����߳�

//��������
BOOL initServer(void);	//��ʼ��������
void initMember(void);	//��ʼ��ȫ�ֱ���
bool initSocket(void);	//��ʼ���������׽���
void exitServer(void);	//�ͷ���Դ
bool startService(void);	//����������
void inputAndOutput(void);	//��������
void showServerStartMsg(BOOL bSuc);	//��ʾ������Ϣ
void showServerExitMsg(void);	//��ʾ�˳���Ϣ
void handleData(char* str);	//���ݴ���
void showTipMsg(int bFirstInput);	//��ʾ������ʾ��Ϣ
BOOL createCleanAndAcceptThread(void);	//������غ���
DWORD __stdcall acceptThread(void* pParam);	//�����ͻ��������߳�
DWORD __stdcall cleanThread(void* pParam);	//�����߳�


