#include "NetCommandHelper.h"
#include "user_helper/UserHelper.h"
#include "items/Bag.h"
#include "network/protocol.h"
#include "network/protocol_message.pb.h"
#include "network/net/NetworkThreadProc.h"
#include "network/net/NetworkWaitingLayer.h"
#include "events/MsgDefine.h"
#include "configuration/ItemExchangeConfig.h"



#define CHECK_PARSE_RSP_DATA(b) if(!(b)){\
	CCAssert(false,"parse rsp data error");\
	return;\
}

NetCommandHelper* NetCommandHelper::getInstance()
{
	static NetCommandHelper* s_netCommandHelper = nullptr;
	if (s_netCommandHelper == nullptr)
	{
		s_netCommandHelper = new NetCommandHelper();
		s_netCommandHelper->init();
	}
	return s_netCommandHelper;
}

NetCommandHelper::NetCommandHelper(void)
	: m_lastErrorCode(NetErrorCode::NetSuccess)
{
}

bool NetCommandHelper::init()
{
	NetworkPackageDispatch::getInstance()->setMsgProc(this);
	return true;
}

void NetCommandHelper::postMessage(const char* msgName, int errCode, void *data)
{
	EventCustom evnt(msgName);
	MsgData msg_data;
	msg_data.code = static_cast<MsgData::MsgCode>(errCode);
	msg_data.data = data;
	evnt.setUserData(&msg_data);
	Director::getInstance()->getEventDispatcher()->dispatchEvent(&evnt);
}


//注册账号
void NetCommandHelper::registerFromGameServer(const char* account, const char* passwd, const char* userName)
{
	const AcountInfo* pAcountInfo = UserHelper::getInstance()->getAcountInfo();
	if (pAcountInfo != nullptr && pAcountInfo->userId != AcountInfo::INVALID_USER_ID)
	{
		CCASSERT(false, "already registed");
		return;
	}

	CCLOG("INFO: %s ---> acount[%s], passwd[%s], name[%s]", __FUNCTION__, account, passwd, userName);
	PetOnline::RegistReq req;
	req.set_account(account);
	req.set_passwd(passwd);
	req.set_nickname(userName);
	AcountInfo temp;
	temp.acountId = account;
	temp.password = passwd;
	temp.userName = userName;
	UserHelper::getInstance()->setAcountInfo(temp);

	std::string sendData = req.SerializeAsString();
	NetworkThreadProc::getInstance()->sendData(Req_Regist, sendData.c_str(), sendData.length());
	NetworkWaitingLayer::showWaiting(true);
}

void NetCommandHelper::onRegistRsp(void* data, unsigned int dataLength)
{
	int rsp_code = MessageErrorCode::Error;
	::PetOnline::RegistRsp rsp;	
	if (data){
		CHECK_PARSE_RSP_DATA ( rsp.ParseFromArray(data, dataLength) );
		rsp_code = rsp.rsp_code();
	}
	else {
		rsp_code = MessageErrorCode::Error;
	}

	CCLOG("INFO: %s ---> rsp_code = %d", __FUNCTION__, rsp_code);
	if (rsp_code != MessageErrorCode::Success)
	{
		this->postMessage(MSG_NETWORK_REGIST, static_cast<int>(MsgData::MsgCode::kNetworkError), nullptr);
		return;
	}

	CCASSERT(rsp.has_user_id(), "error data");
	if (! rsp.has_user_id())
	{
		CCLOG("ERROR: %s ---> regist success, but not has user id", __FUNCTION__);
		this->postMessage(MSG_NETWORK_REGIST, static_cast<int>(MsgData::MsgCode::kNetworkError), nullptr);
		return;
	}
	CCLOG("   --- userId[%d]", rsp.user_id());
	AcountInfo acountInfo = *UserHelper::getInstance()->getAcountInfo();
	acountInfo.userId = rsp.user_id();
	UserHelper::getInstance()->setAcountInfo(acountInfo);

	this->postMessage(MSG_NETWORK_REGIST, static_cast<int>(MsgData::MsgCode::kSuccess), &rsp);
	NetworkWaitingLayer::hideWaiting();
}

//登录
void NetCommandHelper::loginGameServer()
{
	const AcountInfo* pAcountInfo = UserHelper::getInstance()->getAcountInfo();
	if (pAcountInfo == nullptr || pAcountInfo->userId == AcountInfo::INVALID_USER_ID)
	{
		this->postMessage(MSG_NETWORK_LOGIN, static_cast<int>(MsgData::MsgCode::kNetNotRegist), nullptr);
		return;
	}

// 	Device::openNativeMap(300, 300);
// 	auto listener = EventListenerNativeMap::create( CC_CALLBACK_2(SceneMapPick::onNativeMap, this) );
// 	Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

	CCLOG("INFO: %s ---> userId[%d], userName[%s], acountId[%s], passwd[%d]", __FUNCTION__, 
		pAcountInfo->userId, pAcountInfo->userName.c_str(), pAcountInfo->acountId.c_str(), pAcountInfo->password.c_str());
	::PetOnline::LoginReq req;
	req.set_user_id(pAcountInfo->userId);
	::PetOnline::CoordinateInfo* pCoord = req.mutable_coordinate();
	pCoord->set_latitude(100.1);
	pCoord->set_longitude(100.1);

	std::string sendData = req.SerializeAsString();
	NetworkThreadProc::getInstance()->sendData(Req_Login, sendData.c_str(), sendData.length());
	NetworkWaitingLayer::showWaiting(true);
}

void NetCommandHelper::onLoginRsp(void* data, unsigned int dataLength)
{
	int rsp_code = MessageErrorCode::Error;
	::PetOnline::LoginRsp rsp;	
	if (data){
		CHECK_PARSE_RSP_DATA ( rsp.ParseFromArray(data, dataLength) );
		rsp_code = rsp.rsp_code();
	}
	else {
		rsp_code = MessageErrorCode::Error;
	}

	CCLOG("INFO: %s ---> rsp_code = %d", __FUNCTION__, rsp_code);
	if (rsp_code != MessageErrorCode::Success)
	{
		this->postMessage(MSG_NETWORK_LOGIN, static_cast<int>(MsgData::MsgCode::kNetworkError), nullptr);
		return;
	}

	Bag* pBag = UserHelper::getInstance()->getBag();
	//
	int exchangedCount = rsp.exchanged_size();
	CCLOG("INFO: %s ---> exchanged item[%d]", __FUNCTION__, exchangedCount);
	for (int i=0; i<exchangedCount; ++i)
	{
		const ::PetOnline::ExchangeInfo& oneExchangeInfo = rsp.exchanged(i);
		CCASSERT(oneExchangeInfo.is_exchanged(), "error data");
		//if (oneExchangeInfo.is_exchanged())
		{
			//提交交换申请的时候就已经减去了。
			//pBag->reduceItem()
			pBag->addItem(static_cast<ItemId>(oneExchangeInfo.need_item().item_id()), oneExchangeInfo.need_item().item_count());
			CCLOG("   --- itemId[%d], count[%d]", oneExchangeInfo.need_item().item_id(), oneExchangeInfo.need_item().item_count());
		}
	}
	int unshelveCount = rsp.unshelve_size();
	CCLOG("INFO: %s ---> unshelve item[%d]", __FUNCTION__, unshelveCount);
	for (int i=0; i<unshelveCount; ++i)
	{
		const ::PetOnline::ExchangeInfo& oneUnshelveInfo = rsp.unshelve(i);
		//提交交换申请的时候就已经减去了。
		CCASSERT(!oneUnshelveInfo.is_exchanged(), "eeror data");
		//if (!oneUnshelveInfo.is_exchanged())
		{
			pBag->addItem(static_cast<ItemId>(oneUnshelveInfo.onhand_item().item_id()), oneUnshelveInfo.onhand_item().item_count());
			CCLOG("   --- itemId[%d], count[%d]", oneUnshelveInfo.onhand_item().item_id(), oneUnshelveInfo.onhand_item().item_count());
		}
	}

	this->postMessage(MSG_NETWORK_LOGIN, static_cast<int>(MsgData::MsgCode::kSuccess), &rsp);
	NetworkWaitingLayer::hideWaiting();
}

//互换  (useItemId用于交换的item， needItemId交换到的item， needItemCount想要交换多少个)
void NetCommandHelper::exchangeItem(int usedItemId, int changedItemId, int changedItemCount)
{
	const AcountInfo* pAcountInfo = UserHelper::getInstance()->getAcountInfo();
	if (pAcountInfo == nullptr || pAcountInfo->userId == AcountInfo::INVALID_USER_ID)
	{
		this->postMessage(MSG_NETWORK_EXCHANGE, static_cast<int>(MsgData::MsgCode::kNetNotRegist), nullptr);
		return;
	}

	const ItemExchangeRatio* pExchangeInfo = ItemExchangeConfig::getInstance()->getOneExchangeRatio(usedItemId, changedItemId);	
	if (pExchangeInfo == nullptr)
	{
		CCASSERT(false, "configuration error");
		return;
	}
	int needUsedItemCount = 0xffff;
	if (pExchangeInfo->item1Id == usedItemId)
	{
		needUsedItemCount = pExchangeInfo->item1Count * changedItemCount / pExchangeInfo->item2Count;
	}
	else if (pExchangeInfo->item2Id == usedItemId)
	{
		needUsedItemCount = pExchangeInfo->item2Count * changedItemCount / pExchangeInfo->item1Count;
	}
	else
	{
		CCASSERT(false, "configuration error");
		return;
	}

	CCLOG("INFO: %s ---> useItemId[%d], useItemCount[%d], wantedItemId[%d], wantedItemCount[%d]", __FUNCTION__, 
		usedItemId, needUsedItemCount, changedItemId, changedItemCount);

	BagItem* pUsedItem = UserHelper::getInstance()->getBag()->getItem(static_cast<ItemId>(usedItemId));
	if (pUsedItem == nullptr || pUsedItem->count < needUsedItemCount)
	{
		this->postMessage(MSG_NETWORK_EXCHANGE, static_cast<int>(MsgData::MsgCode::kItemNotEnough), nullptr);
		return;
	}

	//提交请求的时候就要扣除
	Bag* pBag = UserHelper::getInstance()->getBag();
	pBag->reduceItem(static_cast<ItemId>(usedItemId), needUsedItemCount);

	::PetOnline::ExchangeReq req;
	req.set_user_id(pAcountInfo->userId);
	::PetOnline::ItemInfo* pUseItem = req.mutable_onhand_item();
	pUseItem->set_item_id(usedItemId);
	pUseItem->set_item_count(needUsedItemCount);
	::PetOnline::ItemInfo* pChangeItem = req.mutable_need_item();
	pChangeItem->set_item_id(changedItemId);
	pChangeItem->set_item_count(changedItemCount);

	std::string sendData = req.SerializeAsString();
	NetworkThreadProc::getInstance()->sendData(Req_Exchange, sendData.c_str(), sendData.length());
	NetworkWaitingLayer::showWaiting(true);
}

void NetCommandHelper::onExchangeRsp(void* data, unsigned int dataLength)
{
	int rsp_code = MessageErrorCode::Error;
	::PetOnline::ExchangeRsp rsp;	
	if (data){
		CHECK_PARSE_RSP_DATA ( rsp.ParseFromArray(data, dataLength) );
		rsp_code = rsp.rsp_code();
	}
	else {
		rsp_code = MessageErrorCode::Error;
	}

	CCLOG("INFO: %s ---> rsp_code = %d", __FUNCTION__, rsp_code);
	if (rsp_code != MessageErrorCode::Success)
	{
		this->postMessage(MSG_NETWORK_EXCHANGE, static_cast<int>(MsgData::MsgCode::kNetworkError), nullptr);
		return;
	}

	Bag* pBag = UserHelper::getInstance()->getBag();
	if (rsp.has_ex_info())
	{
		pBag->addItem(static_cast<ItemId>(rsp.ex_info().onhand_item().item_id()), rsp.ex_info().onhand_item().item_count());
		CCLOG("   --- changed itemId[%d], count[%d]", rsp.ex_info().onhand_item().item_id(), rsp.ex_info().onhand_item().item_count());
	}
	else
	{
		CCLOG("   --- not exchanged, will shelved");
	}

	this->postMessage(MSG_NETWORK_EXCHANGE, static_cast<int>(MsgData::MsgCode::kSuccess), &rsp);
	NetworkWaitingLayer::hideWaiting();
}



void NetCommandHelper::onMessageProcess(NetErrorCode code, MessageCmd cmd, void* data, unsigned int dataLength)
{
	if (code != NetErrorCode::NetSuccess)
	{
		this->setLastError(code);
	}

	switch (cmd)
	{
	case Rsp_Regist:
		this->onRegistRsp(data, dataLength);
		break;
	case Rsp_Login:
		this->onLoginRsp(data, dataLength);
		break;
	case Rsp_Exchange:
		this->onExchangeRsp(data, dataLength);
		break;
	default:
		CCASSERT(false, "error net message cmd");
		break;
	}
}
