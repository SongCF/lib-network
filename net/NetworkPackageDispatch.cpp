#include "network/net/NetworkPackageDispatch.h"
#include "network/net/NetworkThreadProc.h"


using namespace cocos2d;

NetworkPackageDispatch* NetworkPackageDispatch::m_packageDispatch = nullptr;

NetworkPackageDispatch::NetworkPackageDispatch()
:m_msgProc(nullptr)
{
}

NetworkPackageDispatch::~NetworkPackageDispatch()
{

}

NetworkPackageDispatch* NetworkPackageDispatch::getInstance()
{
	if (! m_packageDispatch) {
		m_packageDispatch = new NetworkPackageDispatch;
		m_packageDispatch->initNode();
	}
	return m_packageDispatch;
}

void NetworkPackageDispatch::setMsgProc(NetworkMessageListener* func)
{
	m_msgProc = func;
}

void NetworkPackageDispatch::initNode()
{
	Node::init();

	// Let's running
	this->onEnter();
	this->onEnterTransitionDidFinish();
}

void NetworkPackageDispatch::onEnter()
{
	Node::onEnter();
	this->scheduleUpdate();
}

void NetworkPackageDispatch::onExit()
{
	Node::onExit();
}

void NetworkPackageDispatch::update(float dt)
{
	RespondPackage* data = NetworkThreadProc::getInstance()->getRecvData(); //remember delete
	if (! data) return;

	switch (data->getType())
	{
		// none --- recv err
		// 全当作超时处理
	case RespondPackage::Type_None:
		break;
	case RespondPackage::Type_Conncet_Suc:
		CCLOG("%s ---> connect success", __FUNCTION__);
		break;
	case RespondPackage::Type_Conncet_Err:
		CCLOG("%s ---> connect error", __FUNCTION__);
		messageDispatch(NetErrorCode::NetError);
		break;
	case RespondPackage::Type_Send_Err:
		CCLOG("%s ---> send error", __FUNCTION__);
		messageDispatch(NetErrorCode::NetError);
		break;
	case RespondPackage::Type_Recv_Err:
		CCLOG("%s ---> recv error", __FUNCTION__);
		messageDispatch(NetErrorCode::NetDataError);
		break;
	case RespondPackage::Type_Recv_OK:
		CCLOG("%s ---> recv ok", __FUNCTION__);
		messageDispatch(NetErrorCode::NetSuccess,data);
		break;
	default:
		CCLOG("%s ---> error branch", __FUNCTION__);
		break;
	}

	delete data;
}

void NetworkPackageDispatch::messageDispatch(NetErrorCode errCode, RespondPackage* package)
{
	std::unique_lock<std::mutex> lock;  //析构会unlock
	std::vector<RequestPackage*>* alreadySendQueue = NetworkThreadProc::getInstance()->getAlreadySendQueue(lock);

	if (m_msgProc)
	{
		if (errCode == NetErrorCode::NetSuccess)
		{
			SPHead* pHead = (SPHead*)package->getHead();

			RequestPackage* temp = nullptr;
			for (int iAlreadyQueue=0,countAlreadyQueue=alreadySendQueue->size(); iAlreadyQueue<countAlreadyQueue; ++iAlreadyQueue)
			{
				if (getRspcmdByReqcmd( alreadySendQueue->at(iAlreadyQueue)->getCmdType() ) == pHead->cmd)
				{
					temp = alreadySendQueue->at(iAlreadyQueue);
					alreadySendQueue->erase(alreadySendQueue->begin()+iAlreadyQueue);
					CCLOG("%s ---> find cmd[%d] from already queue", __FUNCTION__, static_cast<int>(temp->getCmdType()));
					break;
				}
			}
			if (temp)
			{
				temp->release();
			}

			m_msgProc->onMessageProcess(NetErrorCode::NetSuccess, (MessageCmd)pHead->cmd, package->getBody(), pHead->data_len-sizeof(unsigned int));  // 除去校验码
		}
		else
		{
			for (int iAlreadyQueue=0,countAlreadyQueue=alreadySendQueue->size(); iAlreadyQueue<countAlreadyQueue; ++iAlreadyQueue)
			{
				RequestPackage* temp = alreadySendQueue->at(iAlreadyQueue);
				m_msgProc->onMessageProcess(errCode, getRspcmdByReqcmd(temp->getCmdType()), 0, 0);
				temp->release();
			}
			alreadySendQueue->clear();
		}
	}
}
