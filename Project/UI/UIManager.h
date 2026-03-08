#ifndef _UI_MANAGER_H_
#define _UI_MANAGER_H_
#include "../DesignPatterns/singleton.h"
#include "../InputManager.h"
#include "UIElement.h"
#include <vector>
#include <queue>

/*
	Manages its UI objects:
	- Detects what UI is hovered/clicked and alerts the UI
	- Prevents overlapping UI from both responding.
*/
class UIManager : public Singleton<UIManager>, Input::InputSub
{
	friend Singleton<UIManager>;
public:
	void Init();

	//Registers an element with the manager.
	//Element should be new'd.
	void Register(UIElement* el);

	//Check for hover
	void Update();

	//Callback for mouse clicks
	void SubscriptionAlert(Input::InputKeyData data) override;

	UIElement& FetchOrphanUI();

private:
	std::vector<UIElement*> elements{};
	std::queue<UIElement*> regQueue{};
	//Prevent registration during the alert loop.
	bool pauseRegistration{ false };

	void HandleRegQueue();
	bool IsOverUI(UIElement* el);
	~UIManager();
};

#endif // !_UI_MANAGER_H_
