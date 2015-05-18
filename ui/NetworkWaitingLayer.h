#ifndef __NETWORK_WAITING_LAYER_H__
#define __NETWORK_WAITING_LAYER_H__

#include "cocos2d.h"

using namespace cocos2d;

#define ZORDER_WAITING_LAYER 10000
#define TAG_NAME_WAITING_LAYER "tag_name_waiting_layer"

class NetworkWaitingLayer : public LayerColor
{
public:
	static void showWaiting(bool showWaitingSprite=true);//显示转圈否
	static void hideWaiting();


	static NetworkWaitingLayer* create(bool bShowWaitingUI=true);
	bool init(bool bShowWaitingUI);
	void setSwallowTouch(bool bSwallow=true);

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool onTouchBegan(Touch *pTouch, Event *pEvent) override;
	virtual void onTouchMoved(Touch *touch, Event *unused_event) override;
	virtual void onTouchEnded(Touch *touch, Event *unused_event) override;

protected:
	bool m_bSwallowTouch;
	EventListenerTouchOneByOne *m_touchListener;
};

#endif /* define(__NETWORK_WAITING_LAYER_H__) */