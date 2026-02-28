#include "UIManager.h"
#include "../InputManager.h"
#include "../Helpers/CollisionUtils.h"
#include <algorithm>

void UIManager::Init() {
	InputManager::GetInstance()->SubscribeMouse(this, 100);
	elements.reserve(10);
}

void UIManager::Register(UIElement* el)
{
	for (UIElement* a : elements) {
		if (el == a) return;
	}
	if (pauseRegistration) {
		regQueue.push(el);
		return;
	}
	el->SetEnabled();
	elements.push_back(el);
	std::sort(elements.begin(), elements.end(), [](UIElement* a, UIElement* b) {return a->GetZ() > b->GetZ();});
}

void UIManager::Update() {
	pauseRegistration = true;

	//Run callback of the clicked elements
	for (UIElement* ab : elements) {
		//Check if over this element.
		if (!ab->IsEnabled() || !IsOverUI(ab)) continue;

		ab->OnHover();
		if (ab->DoesBlockInteraction()) break;
	}

	//End, check registrations
	HandleRegQueue();
}

void UIManager::SubscriptionAlert(Input::InputKeyData data)
{
	if (data.key != AEVK_LBUTTON) return;

	pauseRegistration = true;

	//Run callback of the clicked elements
	for (UIElement* ab : elements) {
		//Check if over this element.
		if (!ab->IsEnabled() || !IsOverUI(ab)) continue;

		if (data.type == Input::INPUT_TYPE::TRIGGERED) {
			ab->OnClick();
		}
		else if (data.type == Input::INPUT_TYPE::RELEASED) {
			ab->OnRelease();
		}
		else ab->OnHover(true); //Held

		//If element blocks interaction, stop looping
		if (ab->DoesBlockInteraction()) {
			data.consume = true;
			break;
		}
	}

	//End, check registrations
	HandleRegQueue();
}

UIManager::~UIManager()
{
	for (UIElement* a : elements) {
		delete a;
	}
}

void UIManager::HandleRegQueue()
{
	pauseRegistration = false;
	while (!regQueue.empty()) {
		Register(regQueue.front());
		regQueue.pop();
	}
}

bool UIManager::IsOverUI(UIElement* el)
{
	return (el->GetShape() == Collision::COL_RECT && IsCursorOver(el->GetPos(), el->GetSize().x, el->GetSize().y))
		|| (el->GetShape() == Collision::COL_CIRCLE && IsCursorOverOval(el->GetPos(), el->GetSize(), 0));
}
