#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_
#include "DesignPatterns/singleton.h"
#include "DesignPatterns/Subscriber.h"
#include "AEEngine.h"
#include <vector>
#include <array>

namespace Input {
	enum class INPUT_TYPE : char {
		CURR,
		TRIGGERED,
		RELEASED,

		SCROLL_UP,
		SCROLL_DOWN
	};

	struct InputKeyData {
		u8 key;
		INPUT_TYPE type;
		//Set to true to block other subsequent elements from responding to this key.
		bool& consume;
	};

	//Alerts when certain keys are pressed.
	//NOTE: Always check the key, as alert may be called for every registered key.
	struct InputSub : Subscriber<Input::InputKeyData> {
		void SubscriptionAlert(Input::InputKeyData content) = 0;
	};
}

class InputManager : public Singleton<InputManager>
{
	friend Singleton<InputManager>;
public:
	void Init();
	//Check input
	void Update();

	//Subscribe to receive keyboard input.
	//Priority: If multiple objects respond to the same key press, the one with higher priority gets the alert first
	InputManager& SubscribeKeyboard(Input::InputSub* sub, int priority = 0);

	//Subscribe to receive mouse input (click / release) from LMB, RMB and middle btn.
	//Priority: If multiple objects respond to the same key press, the one with higher priority gets the alert first
	InputManager& SubscribeMouse(Input::InputSub* sub, int priority = 0);

	//Let manager know to poll this key (keyboard only).
	InputManager& Key(u8 key);

	void Enable(bool enable = true) { enabled = enable; }
	bool IsEnabled() const { return enabled; }

	int GetMinPrio() const { return minPrio; }
	// The minimum priority level for objects to receive callbacks.
	// Recommended to save the previous value and set back
	void SetMinPrio(int min) { minPrio = min; }

private:
	bool enabled{true};
	//Receiver must have priority >= this for it to receive a callback
	int minPrio{-1000};
	struct Receiver {
		Input::InputSub* sub;
		int priority;
	};
	std::vector<Receiver> receivers{};
	std::vector<Receiver> receiversMouse{};

	std::vector<u8> keys{};

	std::array<u8, 3> mouseKeys{ AEVK_LBUTTON, AEVK_RBUTTON, AEVK_MBUTTON };
};


#endif // !_INPUT_MANAGER_H_
