#ifndef __COMMAND_TEST_LAYER_H__
#define __COMMAND_TEST_LAYER_H__


#include "cocos2d.h"
using namespace cocos2d;

class CommandTestLayer : public LayerColor
{
public:
	CREATE_FUNC(CommandTestLayer);
	bool init();
	void drawCommandItemList();

protected:
	void onButtonClicked(Ref* obj);

private:
	Menu* m_pMenu;
};



#endif /* define(__COMMAND_TEST_LAYER_H__) */