#ifndef __NET_CONMAND_HELPER_H__
#define __NET_CONMAND_HELPER_H__


#include "cocos2d.h"
using namespace cocos2d;

#include <string>
#include "network/net/NetworkPackageDispatch.h"


class NetCommandHelper : public NetworkMessageListener
{
public:
	static NetCommandHelper* getInstance();

	//注册账号
	void registerFromGameServer(const char* account, const char* passwd, const char* userName);
	//登录
	void loginGameServer();
	//互换  (useItemId用于交换的item， changedItemId交换到的item， changedItemCount想要交换多少个)
	void exchangeItem(int usedItemId, int changedItemId, int changedItemCount);

protected:
	NetCommandHelper(void);
	bool init();
	void postMessage(const char* msgName, int errCode, void *data);
	void setLastError(NetErrorCode errcode){m_lastErrorCode = errcode;}
	NetErrorCode GetLastError(){return m_lastErrorCode;}

	virtual void onMessageProcess(NetErrorCode, MessageCmd, void*, unsigned int);
	void onRegistRsp(void* data, unsigned int dataLength);
	void onLoginRsp(void* data, unsigned int dataLength);
	void onExchangeRsp(void* data, unsigned int dataLength);

private:
	NetErrorCode m_lastErrorCode;
};

#endif /* define(__NET_CONMAND_HELPER_H__) */