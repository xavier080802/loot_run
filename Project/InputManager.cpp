#include "InputManager.h"
#include <algorithm>
#include <iostream>

namespace {
	//Checks if k is pressed/triggered/released, and sets out to the type.
	//Returns whether the key was interacted with or not, in that case out is not changed.
	bool CheckType(u8 k, Input::INPUT_TYPE& out) {
		using Input::INPUT_TYPE;
		if (AEInputCheckTriggered(k)) out = INPUT_TYPE::TRIGGERED;
		else if (AEInputCheckCurr(k)) out = INPUT_TYPE::CURR;
		else if (AEInputCheckReleased(k)) out = INPUT_TYPE::RELEASED;
		else return false;

		return true;
	}
}

void InputManager::Init() {
	receivers.reserve(5);
	keys.reserve(10);
}

void InputManager::Update() {
	using Input::INPUT_TYPE;
	//Mouse
	for (u8 m : mouseKeys) {
		INPUT_TYPE type{};
		if (!CheckType(m, type)) continue;

		bool consumed{ false };
		for (Receiver const& r : receiversMouse) {
			r.sub->SubscriptionAlert({ m, type, consumed });
			if (consumed) break;
		}
	}
	s32 scroll{};
	AEInputMouseWheelDelta(&scroll);
	if (scroll) {
		bool consumed{ false };
		for (Receiver const& r : receiversMouse) {
			r.sub->SubscriptionAlert({ VK_SCROLL, scroll > 0 ? Input::INPUT_TYPE::SCROLL_UP : Input::INPUT_TYPE::SCROLL_DOWN, consumed });
			if (consumed) break;
		}
	}

	//Keyboard
	for (u8 k : keys) {
		INPUT_TYPE type{};
		if (!CheckType(k, type)) continue;

		bool consumed{ false };
		for (Receiver const& r : receivers) {
			r.sub->SubscriptionAlert({ k, type, consumed });
			if (consumed) break;
		}
	}
}

InputManager& InputManager::SubscribeKeyboard(Input::InputSub* sub, int priority)
{
	for (Receiver const& r : receivers) {
		if (r.sub == sub) return *this;
	}
	receivers.push_back({ sub, priority });
	//Sort in descending priority.
	std::sort(receivers.begin(), receivers.end(), [](Receiver const& a, Receiver const& b) {return a.priority > b.priority;});
	return *this;
}

InputManager& InputManager::SubscribeMouse(Input::InputSub* sub, int priority)
{
	for (Receiver const& r : receiversMouse) {
		if (r.sub == sub) return *this;
	}
	receiversMouse.push_back({ sub, priority });
	//Sort in descending priority.
	std::sort(receiversMouse.begin(), receiversMouse.end(), [](Receiver const& a, Receiver const& b) {return a.priority > b.priority;});
	return *this;
}

InputManager& InputManager::Key(u8 key)
{
	if (key == AEVK_LBUTTON || key == AEVK_RBUTTON || key == AEVK_MBUTTON) return *this;
	for (u8 k : keys) {
		if (k == key)return *this;
	}
	keys.push_back(key);
	return *this;
}

