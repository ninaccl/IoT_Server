#pragma once
#include <winsock2.h>
#include <iostream>

using namespace std;

#define TIMEFOR_THREAD_CLIENT 500	//线程睡眠时间

#define	MAX_NUM_CLIENT 10	//接受的客户端连接最多数量
#define	MAX_NUM_BUF 64	//缓冲区的最大长度
#define PING "ping"	 //发送标志

class CClient
{
public:
	CClient(const SOCKET sClient, const sockaddr_in &addrClient);
	virtual ~CClient();

public:
	sockaddr_in GetAddr(void)	//返回addr
	{
		return m_addr;
	}
	BOOL StartRuning(void);	//创建发送和接收数据线程
	BOOL IsConning(void)	//是否连接存在
	{					
		return m_bConning;
	}
	void DisConning(void)	//断开与客户端的连接
	{					
		m_bConning = FALSE;
	}
	BOOL IsSend(void) 
	{
		m_bSend = TRUE;
		return m_bSend;
	}

public:
	static DWORD __stdcall RecvDataThread(void* pParam);	//接收客户端数据
	static DWORD __stdcall SendDataThread(void* pParam);	//向客户端发送数据

private:
	CClient();
private:
	SOCKET m_socket;	//套接字
	sockaddr_in m_addr;	//地址
	HANDLE m_hEvent;	//事件对象
	HANDLE m_hThreadSend;	//发送数据线程句柄
	HANDLE m_hThreadRecv;	//接收数据线程句柄
	CRITICAL_SECTION m_cs;	//临界区对象
	BOOL m_bConning;	//客户端连接状态
	BOOL m_bSend;	 //数据发送状态
};
