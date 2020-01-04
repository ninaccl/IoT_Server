#pragma once
#include <winsock2.h>
#include <iostream>

using namespace std;

#define TIMEFOR_THREAD_CLIENT 500	//�߳�˯��ʱ��

#define	MAX_NUM_CLIENT 10	//���ܵĿͻ��������������
#define	MAX_NUM_BUF 64	//����������󳤶�

class CClient
{
public:
	CClient(const SOCKET sClient, const sockaddr_in &addrClient);
	virtual ~CClient();

public:
	BOOL StartRuning(void);	//�������ͺͽ��������߳�
	BOOL IsConning(void)	//�Ƿ����Ӵ���
	{					
		return m_bConning;
	}
	void DisConning(void)	//�Ͽ���ͻ��˵�����
	{					
		m_bConning = FALSE;
	}
	BOOL IsSend(void) 
	{
		m_bSend = TRUE;
		return m_bSend;
	}

public:
	static DWORD __stdcall RecvDataThread(void* pParam);	//���տͻ�������
	static DWORD __stdcall SendDataThread(void* pParam);	//��ͻ��˷�������

private:
	CClient();
private:
	SOCKET m_socket;	//�׽���
	sockaddr_in m_addr;	//��ַ
	HANDLE m_hEvent;	//�¼�����
	HANDLE m_hThreadSend;	//���������߳̾��
	HANDLE m_hThreadRecv;	//���������߳̾��
	CRITICAL_SECTION m_cs;	//�ٽ�������
	BOOL m_bConning;	//�ͻ�������״̬
	BOOL m_bSend;	 //���ݷ���״̬
};
