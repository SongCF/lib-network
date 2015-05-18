/*
 * define file about portable socketclass. 
 * description:this sock is suit bothwindows and linux
 * design:odison
 * e-mail:odison@126.com>
 * 
 */

#ifndef __ODSOCKET_H__
#define __ODSOCKET_H__

#ifdef WIN32
		#include<winsock2.h>
        //#include<winsock.h>
        typedef int                socklen_t;
#else
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <netdb.h>
        #include <fcntl.h>
        #include <unistd.h>
        #include <sys/stat.h>
        #include <sys/types.h>
        #include <arpa/inet.h>
		#include <errno.h>
        typedef int                SOCKET;

        #pragma region define win32const variable inlinux
        #define INVALID_SOCKET       -1
        #define SOCKET_ERROR         -1
        #pragma endregion
#endif


class ODSocket {

public:
        ODSocket(SOCKET sock =INVALID_SOCKET);
        ~ODSocket();

        // Create socket object for snd/recv data
        bool Create(int af, int type,int protocol =0);

        // Connectsocket
        bool Connect(const char* ip,unsigned short port);
#pragma region Server
        // Bindsocket for Server
        bool Bind(unsigned short port);

        // Listensocket  for Server
        bool Listen(int backlog = 5);

        // Accept socket  for Server
        bool Accept(ODSocket& s,char* fromip =NULL);
#pragma endregion
        
		//成功则返回0。否则的话，返回SOCKET_ERROR
		int SetSendTimeOut(int t_m);
		int SetRecvTimeOut(int t_m);

		// 检测网络连接状态，如果断开了就返回-1
		int CheckLinkState();

        // Send socket
        int Send(const char* buf, int len, int flags =0);

        // Recv socket
        int Recv(char* buf, int len,int flags =0);
        
        // Close socket
        int Close();

        // Get errno
        int GetError();
        
        #pragma region just for win32
        // Init winsock DLL, just for win32
        static int Init();        
        // Clean winsockDLL, just for win32
        static int Clean();
        #pragma endregion

        // Domain parse
        static bool DnsParse(const char* domain, char*ip);

        ODSocket& operator =(SOCKET s);

        operator SOCKET();

		SOCKET getSock(){return m_sock;}

protected:
        SOCKET m_sock;
};

#endif /* define(__ODSOCKET_H__) */


