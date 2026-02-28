#ifndef _UI_MANAGER_H_
#define _UI_MANAGER_H_
#include "../DesignPatterns/singleton.h"

/*
	Manages its UI objects:
	- Detects what UI is hovered/clicked and alerts the UI
*/
class UIManager : public Singleton<UIManager>
{
	friend Singleton<UIManager>;
public:
	void Init();

private:

};

#endif // !_UI_MANAGER_H_
