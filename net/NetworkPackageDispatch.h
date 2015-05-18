#ifndef __NETWORK_MESSAGE_LISTENER_H__
#define __NETWORK_MESSAGE_LISTENER_H__

#include "network/net/NetworkThreadProc.h"

#include "cocos2d.h"


enum class NetErrorCode
{
	NetSuccess = 0,
	NetError = 1,
	NetTimeOut = 2,
	NetDataError = 3,
};

class NetworkMessageListener
{
public:
	virtual void onMessageProcess(NetErrorCode, MessageCmd, void*, unsigned int) = 0;
};



class NetworkPackageDispatch : public cocos2d::Node
{
private:
	static NetworkPackageDispatch* m_packageDispatch;
	NetworkPackageDispatch();
	void initNode();
public:
	static NetworkPackageDispatch* getInstance();
	~NetworkPackageDispatch();
	void setMsgProc(NetworkMessageListener* listener);

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void update(float dt) override;
	void messageDispatch(NetErrorCode errCode, RespondPackage* package=nullptr);

private:
	NetworkMessageListener* m_msgProc;
};

#endif /* define(__NETWORK_MESSAGE_LISTENER_H__) */