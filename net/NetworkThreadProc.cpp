#include "network/net/NetworkThreadProc.h"
#include "cocos2d.h"   //log


#define DOMAIN_ENABLE 0
#define DOMAIN_NAME				"loststar.camellia-network.com"
#define DEFAULT_SERVER_IP		"192.168.2.10"
#define DEFAULT_SERVER_PORT		23690
#define SEND_TIME_OUT	20000	// 发送超时
#define RECV_TIME_OUT	30000	// 接收超时


NetworkThreadProc * NetworkThreadProc::_networkThreadProc = nullptr;

NetworkThreadProc::NetworkThreadProc()
{

}

NetworkThreadProc::~NetworkThreadProc()
{

}

NetworkThreadProc* NetworkThreadProc::getInstance()
{
	if (! _networkThreadProc){
		_networkThreadProc = new NetworkThreadProc;
	}
	return _networkThreadProc;
}

void NetworkThreadProc::initThread()
{
	m_IPAddress = getIPAddress();
	if (m_IPAddress.empty()) {
		m_IPAddress = DEFAULT_SERVER_IP;
	}

	m_networkState = NetworkThreadProc::NetworkStat_NotConnect;
	m_threadSend = std::thread(&NetworkThreadProc::sendThread, this);
	m_threadRecv = std::thread(&NetworkThreadProc::recvThread, this);
}
	//send data
void NetworkThreadProc::sendData(MessageCmd type, const char* msg_body, int body_length)
{
	//组包
	RequestPackage *pack = new RequestPackage(type, msg_body, body_length);

	//阻塞直到获得锁。
	std::unique_lock<std::mutex> lock(m_mutexSendBuf);
	m_sendBuf.push(pack); //pop之后delete
	m_sendBufCondVar.notify_one();
}

	//recv dispatch data
RespondPackage* NetworkThreadProc::getRecvData()
{
	//尝试获得引用的互斥锁，未能获得也不阻塞。
	std::unique_lock<std::mutex> lock(m_mutexRecvBuf, std::try_to_lock);

	if (lock.owns_lock() && m_recvBuf.size()>0)
	{
		RespondPackage *p = m_recvBuf.front();
		m_recvBuf.pop();
		return p;
	}
	return nullptr;
}
std::vector<RequestPackage*>* NetworkThreadProc::getAlreadySendQueue(std::unique_lock<std::mutex>& lock)
{
	lock = std::unique_lock<std::mutex>(m_mutexAlreadySend);
	return &m_alreadySendQueue;
}

void NetworkThreadProc::addRspPackage(RespondPackage* rsp)
{
	CCLOG("INFO: %s ---> %d", __FUNCTION__, static_cast<int>(rsp->getType()));
	std::unique_lock<std::mutex> lock(m_mutexRecvBuf);
	m_recvBuf.push(rsp);
}




std::string NetworkThreadProc::getIPAddress()
{
#if DOMAIN_ENABLE
	char ip[16] = {0};
	if (ODSocket::DnsParse(DOMAIN_NAME, ip)){
		return ip;
	}else {
		return "";
	}
#else
	return DEFAULT_SERVER_IP;
#endif
}

void NetworkThreadProc::sleep(long millisecond)
{
#ifdef WIN32
	Sleep(millisecond);
#else
	usleep(millisecond*1000);
#endif
}

bool NetworkThreadProc::connectToServer()
{
	if (m_networkState == NetworkStat_Connected)
		return true;

    //如果只在network init时初始化会出现问题：
    //断开vpn时，解析出错误的链接不上，当连上vpn后再链接server，如果这里没有重新获取ip，则依然连接不上
    m_IPAddress = getIPAddress();
    if (m_IPAddress.empty()) {
        m_IPAddress = DEFAULT_SERVER_IP;
    }
    
	if (m_socket.Init() == -1)
	{
		CCLOG("%s ---> init error", __FUNCTION__);
		return false;
	}
	if (! m_socket.Create(AF_INET,SOCK_STREAM,0))
	{
		CCLOG("%s ---> create error, err code = %d", __FUNCTION__, m_socket.GetError());
		return false;
	}
	if (! m_socket.Connect(m_IPAddress.c_str(), DEFAULT_SERVER_PORT))
	{
		CCLOG("%s ---> connect error [%s-%d], err code = %d", __FUNCTION__, m_IPAddress.c_str(), DEFAULT_SERVER_PORT, m_socket.GetError());
		closeConnection();
		return false;
	}
	CCLOG("%s ---> connect seccess [%s-%d]", __FUNCTION__, m_IPAddress.c_str(), DEFAULT_SERVER_PORT);

	//设置状态
	m_networkState = NetworkStat_Connected;

	// 设置超时
	m_socket.SetRecvTimeOut(RECV_TIME_OUT);
	m_socket.SetSendTimeOut(SEND_TIME_OUT);

	return true;
}

void NetworkThreadProc::closeNetwork()
{
	closeConnection();
}
void NetworkThreadProc::closeConnection()
{
	if (m_socket.Close() != 0)
	{
		CCLOG("%s ---> Close Error,errCode = %d", __FUNCTION__, m_socket.GetError());
	}
	if (m_socket.Clean() != 0)
	{
		CCLOG("%s ---> Clean Error,errCode = %d", __FUNCTION__, m_socket.GetError());
	}
	//
	m_networkState = NetworkStat_NotConnect;
	CCLOG(" -----Network closed-----");

	int count = 0;
	RequestPackage *temp;
	std::unique_lock<std::mutex> lock(m_mutexSendBuf);
	while(! m_sendBuf.empty())
	{
		++count;
		temp = m_sendBuf.front();
		m_sendBuf.pop();
		delete temp;
	}
	CCLOG("%s --> throwed %d request packege(s)", __FUNCTION__, count);
}


void NetworkThreadProc::sendThread()
{
	while (true)
	{
		RequestPackage *msg = nullptr;

		//1. 是否有数据需要发送
		{
			std::unique_lock<std::mutex> lock(m_mutexSendBuf);
			if (m_sendBuf.empty())
			{
				CCLOG("%s ---> waiting send buffer", __FUNCTION__);
				//1.先释放lock  2.然后等待被唤醒  3.被唤醒后等待获取lock
				m_sendBufCondVar.wait(lock);
			}
			msg = m_sendBuf.front();
			m_sendBuf.pop();
		}
		{
			//放入已发队列 （用于接受异常时判断）
			std::unique_lock<std::mutex> lock_alreadySendQueue(m_mutexAlreadySend);
			m_alreadySendQueue.push_back(msg);
			msg->retain();
		}

		//2. 检测网络连接, 没连上则连接
		if (NetworkThreadProc::NetworkStat_NotConnect == m_networkState)
		{
			if (! connectToServer())
			{
				CCLOG("%s ---> connect to server Fail", __FUNCTION__);
				addRspPackage(new RespondPackage(RespondPackage::Type_Conncet_Err));
				//delete msg;
				msg->release();
				sleep(2000);
				continue;
			}
			// 通知链接成功
			addRspPackage(new RespondPackage(RespondPackage::Type_Conncet_Suc));
//			m_recvBufCondVar.notify_one();
			CCLOG("%s ---> connect to server OK", __FUNCTION__);
		}

		//3. send data
		int len = m_socket.Send(msg->getData(), msg->getDataLength(), 0);
		if (len != msg->getDataLength())
		{
			int socket_errcode = m_socket.GetError();
			CCLOG("%s ---> send data error[%d, %s], real size = %d, send size = %d", __FUNCTION__, socket_errcode, strerror(socket_errcode), msg->getDataLength(), len);
			// 通知发送数据出错
			addRspPackage(new RespondPackage(RespondPackage::Type_Send_Err));
			//delete msg;
			msg->release();
			continue;
		}
		else
		{
			CCLOG("%s ---> send data success", __FUNCTION__);
			//delete msg;
			msg->release();
			continue;
		}
	}
}
void NetworkThreadProc::recvThread()
{
	while (true)
	{
		if (NetworkThreadProc::NetworkStat_Connected == m_networkState)
		{
			CCLOG("%s ---> recv data .....", __FUNCTION__);
			//1. 读取消息头
			char* head = new char[sizeof(SPHead)];
			int len = m_socket.Recv(head,sizeof(SPHead),0);
			if ( len != sizeof(SPHead))
			{
				//服务器一段时间未收到 客户端的消息就会自动断开，此时会收到recv head error!
				int sock_errcode = m_socket.GetError();
				CCLOG("%s ---> recv Head error[%d, %s], total length = %d, readed length = %d", __FUNCTION__, sock_errcode, strerror(sock_errcode), sizeof(SPHead), len);
                addRspPackage(new RespondPackage(RespondPackage::Type_Recv_Err));
                // 出问题了
                closeConnection();
				delete head;
				continue;
			}
			//2. 检测消息头
			if (! isValiedServerHead((SPHead*)head))
			{
				CCLOG("%s ---> recv Head is invalied", __FUNCTION__);
				addRspPackage(new RespondPackage(RespondPackage::Type_Recv_Err));
				// 出问题了
				closeConnection();
				delete head;
				continue;
			}
			CCLOG("%s ---> recv head success!", __FUNCTION__);

			//3. 获取数据包
			int data_len = ntohl(((SPHead*)head)->data_len);
			char* body = new char[data_len];
			CCLOG("%s ---> body length = %d", __FUNCTION__, data_len);
			int len_body = m_socket.Recv(body, data_len, 0);
			if ( len_body != data_len)
			{
				int sock_errcode = m_socket.GetError();
				CCLOG("%s ---> recv body error[%d, %s] , total length = %d, readed length = %d", __FUNCTION__, sock_errcode, strerror(sock_errcode), data_len, len_body);
				addRspPackage(new RespondPackage(RespondPackage::Type_Recv_Err));
				// 出问题了
				closeConnection();
				delete head;
				delete body;
				continue;
			}
			CCLOG("%s ---> recv body success!", __FUNCTION__);

			//4. 校验数据包
			unsigned int recvAdler;
			memcpy(&recvAdler, body+len_body-sizeof(unsigned int), sizeof(unsigned int));
			recvAdler = ntohl(recvAdler);
			unsigned int adler = Adler::adler32(1, (const Bytef*)body, len_body-sizeof(unsigned int));
			if (adler != recvAdler)
			{
				CCLOG("%s ---> recv body error, adler code error", __FUNCTION__);
				addRspPackage(new RespondPackage(RespondPackage::Type_Recv_Err));
				// 出问题了
				closeConnection();
				delete head;
				delete body;
				continue;
			}

			//5. 通知接收数据成功
			addRspPackage(new RespondPackage(RespondPackage::Type_Recv_OK, head, body));


			//will change
			//closeConnection(); //短链接

			continue;
		}
 		else
 		{
// 			std::unique_lock<std::mutex> lock(m_mutexRecvBuf);
// 			m_recvBufCondVar.wait(lock);
            sleep(1000);
 			continue;
 		}
	}
}