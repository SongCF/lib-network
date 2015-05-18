#include "CommandTestLayer.h"
#include "network/NetCommandHelper.h"
#include "items/ItemDefine.h"

//command
struct CommandTable
{
	const char* command;
	std::function<void()> callback;
};

static CommandTable s_commandTableList[] = {
	{"Register From Game Server", [](){NetCommandHelper::getInstance()->registerFromGameServer("TestAcount", "TestPasswd", "TestName");}},
	{"Login Game Server", [](){NetCommandHelper::getInstance()->loginGameServer();}},
	{"Exchange Item", [](){NetCommandHelper::getInstance()->exchangeItem((int)ItemId::kItemFood_Bread, (int)ItemId::kItemFood_Salad, 1);}},
};
static const int s_commandCount = sizeof(s_commandTableList) / sizeof(s_commandTableList[0]);


bool CommandTestLayer::init()
{
	if (! LayerColor::initWithColor(Color4B(0,0,0,160))) return false;

	m_pMenu = Menu::create();
	this->addChild(m_pMenu, 10);
	m_pMenu->setPosition(Vec2::ZERO);	
	drawCommandItemList();

	return true;
}

void CommandTestLayer::drawCommandItemList()
{
	float gap = 32;
	float start_y = Director::getInstance()->getWinSize().height - gap;
	float pos_x = Director::getInstance()->getWinSize().width/2;

	MenuItemFont::setFontSize(28);
	for (int i=0; i<s_commandCount; ++i)
	{
		MenuItemFont *font = MenuItemFont::create(s_commandTableList[i].command, std::bind(&CommandTestLayer::onButtonClicked, this, std::placeholders::_1));
		m_pMenu->addChild(font, 1, 10000 + i);
		font->setPosition(pos_x, start_y);
		start_y -= gap;
	}
}

void CommandTestLayer::onButtonClicked(Ref* obj)
{
	MenuItemFont *PFont = static_cast<MenuItemFont*>(obj);
	int idx = PFont->getTag() - 10000;
	s_commandTableList[idx].callback();
}