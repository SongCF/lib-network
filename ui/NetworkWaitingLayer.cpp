#include "NetworkWaitingLayer.h"


NetworkWaitingLayer* NetworkWaitingLayer::create(bool bShowWaitingUI)
{
	NetworkWaitingLayer* layer = new NetworkWaitingLayer;
	if (layer && layer->init(bShowWaitingUI))
	{
		layer->autorelease();
	}
	else
	{
		CC_SAFE_DELETE(layer);
		layer = nullptr;
	}
	return layer;
}
bool NetworkWaitingLayer::init(bool bShowWaitingUI)
{
	m_bSwallowTouch = true;

	if (! LayerColor::initWithColor(Color4B(0,0,0,80))) return false;

	if (bShowWaitingUI)
	{
		Sprite* s = Sprite::create(); //加入转圈等待的图片
		this->addChild(s);
		s->setPosition(Director::getInstance()->getWinSize()/2);
		s->runAction(RepeatForever::create(RotateBy::create(1, 360)));
	}
	this->setName(TAG_NAME_WAITING_LAYER);
	this->setLocalZOrder(ZORDER_WAITING_LAYER);
	return true;
}

void NetworkWaitingLayer::setSwallowTouch(bool bSwallow)
{
	m_bSwallowTouch = bSwallow;
}

void NetworkWaitingLayer::onEnter()
{
	LayerColor::onEnter();

	m_touchListener = EventListenerTouchOneByOne::create();
	m_touchListener->setSwallowTouches(true);
	m_touchListener->onTouchBegan = std::bind(&NetworkWaitingLayer::onTouchBegan, this, std::placeholders::_1, std::placeholders::_2);
	m_touchListener->onTouchMoved = std::bind(&NetworkWaitingLayer::onTouchMoved, this, std::placeholders::_1, std::placeholders::_2);
	m_touchListener->onTouchEnded = std::bind(&NetworkWaitingLayer::onTouchEnded, this, std::placeholders::_1, std::placeholders::_2);
	Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(m_touchListener, this);
}
void NetworkWaitingLayer::onExit()
{
	Director::getInstance()->getEventDispatcher()->removeEventListener(m_touchListener);
	LayerColor::onExit();
}

bool NetworkWaitingLayer::onTouchBegan(Touch *pTouch, Event *pEvent)
{
	return m_bSwallowTouch;
}

void NetworkWaitingLayer::onTouchMoved(Touch *touch, Event *unused_event)
{
}
void NetworkWaitingLayer::onTouchEnded(Touch *touch, Event *unused_event)
{

}

void NetworkWaitingLayer::showWaiting(bool showWaitingSprite)
{
	NetworkWaitingLayer* pLayer = dynamic_cast<NetworkWaitingLayer*>(Director::getInstance()->getRunningScene()->getChildByName(TAG_NAME_WAITING_LAYER));
	if (pLayer){
		pLayer->setVisible(true);
		pLayer->setSwallowTouch(true);
	}else {
		Director::getInstance()->getRunningScene()->
			addChild(NetworkWaitingLayer::create(showWaitingSprite), ZORDER_WAITING_LAYER, TAG_NAME_WAITING_LAYER);
	}
}
void NetworkWaitingLayer::hideWaiting()
{
	NetworkWaitingLayer* pLayer = dynamic_cast<NetworkWaitingLayer*>(Director::getInstance()->getRunningScene()->getChildByName(TAG_NAME_WAITING_LAYER));
	if (pLayer) {
		pLayer->setVisible(false);
		pLayer->setSwallowTouch(false);
	}
}