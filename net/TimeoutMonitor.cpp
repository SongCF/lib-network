#include "network/net/TimeoutMonitor.h"

using namespace cocos2d;

TimeoutMonitor* TimeoutMonitor::mTimeMonitor = nullptr;

TimeoutMonitor::TimeoutMonitor() 
{
	this->init();
	this->onEnter();
	this->onEnterTransitionDidFinish();
}

TimeoutMonitor* TimeoutMonitor::getInstance()
{
	if (mTimeMonitor == nullptr)
	{
		//MessageBox("new time monitor", "");
		mTimeMonitor = new TimeoutMonitor();
	}
	return mTimeMonitor;
}

void TimeoutMonitor::startMonitor(int msgCmd, float time_wait)
{
	m_curCmd = msgCmd;
	CCLOG("%s:time_wait = %f",__FUNCTION__,time_wait);
	unschedule(schedule_selector(TimeoutMonitor::timeOutCallback));
	schedule(schedule_selector(TimeoutMonitor::timeOutCallback),time_wait,0,0);
}

void TimeoutMonitor::cancleMonitor()
{
	CCLOG("%s",__FUNCTION__);
	unschedule(schedule_selector(TimeoutMonitor::timeOutCallback));
}

void TimeoutMonitor::timeOutCallback(float t)
{
	CCLOG("TimeoutMonitor::---------------TimeOut-----------");
	unschedule(schedule_selector(TimeoutMonitor::timeOutCallback));
	NetworkThreadProc::getInstance()->closeNetwork();


	Node *obj = Node::create();
	obj->retain();
	obj->setTag(m_curCmd);
	Director::getInstance()->getEventDispatcher()->dispatchCustomEvent(MSG_WAIT_TIME_OUT, obj);
	//CCNotificationCenter::sharedNotificationCenter()->postNotification(MSG_WAIT_TIME_OUT,obj);
	obj->release();
}