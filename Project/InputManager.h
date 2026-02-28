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
		RELEASED
	};

	struct InputKeyData {
		u8 key;
		INPUT_TYPE type;
		//Set to true to block other subsequent elements from responding to this key.
		bool& consume;
	};
}

class InputManager : public Singleton<InputManager>
{
	friend Singleton<InputManager>;
public:
	//Alerts when certain keys are pressed.
	//NOTE: Always check the key, as alert may be called for every registered key.
	struct InputSub : Subscriber<Input::InputKeyData> {
		void SubscriptionAlert(Input::InputKeyData content) = 0;
	};

	void Init();
	void Update();

	//Subscribe to receive keyboard input.
	//Priority: If multiple objects respond to the same key press, the one with higher priority gets the alert first
	InputManager& SubscribeKeyboard(InputSub* sub, int priority = 0);

	//Subscribe to receive mouse input.
	//Priority: If multiple objects respond to the same key press, the one with higher priority gets the alert first
	InputManager& SubscribeMouse(InputSub* sub, int priority = 0);

	//Let manager know to poll this key (keyboard only).
	InputManager& Key(u8 key);

private:
	struct Receiver {
		InputSub* sub;
		int priority;
	};
	std::vector<Receiver> receivers{};
	std::vector<Receiver> receiversMouse{};

	std::vector<u8> keys{};

	std::array<u8, 3> mouseKeys{ AEVK_LBUTTON, AEVK_RBUTTON, AEVK_MBUTTON };
};


#endif // !_INPUT_MANAGER_H_
