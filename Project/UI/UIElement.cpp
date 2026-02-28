#include "UIElement.h"
#include "UIManager.h"
#include <json/json.h>

UIElement::UIElement(AEVec2 _pos, AEVec2 _size, int _z, Collision::SHAPE _shape, bool _blockInteraction)
	: pos{ _pos }, size{ _size }, z{ _z }, shape{ _shape }, blocksInteraction{ _blockInteraction }
{
	UIManager::GetInstance()->Register(this);
}

UIElement::UIElement(Json::Value const& json)
{
	if (json.findArray("pos") && json["pos"].size() == 2) {
		Json::Value posArr{ json["pos"] };
		pos = { posArr[0].asFloat(), posArr[0].asFloat() };
	}
	if (json.findArray("size") && json["size"].size() == 2) {
		Json::Value sizeArr{ json["size"] };
		size = { sizeArr[0].asFloat(), sizeArr[0].asFloat() };
	}
	z = json.get("z", 0).asInt();
	blocksInteraction = json.get("blockInteraction", true).asBool();

	UIManager::GetInstance()->Register(this);
}

void UIElement::OnClick()
{
	if (clickCallback) clickCallback();
}

void UIElement::OnHover(bool mouseDown)
{
	if (hoverCallback) hoverCallback(mouseDown);
}

void UIElement::OnRelease()
{
	if (releaseCallback) releaseCallback();
}

UIElement& UIElement::SetClickCallback(std::function<void()> const& _callback)
{
	clickCallback = _callback;
	return *this;
}

UIElement& UIElement::SetReleaseCallback(std::function<void()> const& _callback)
{
	releaseCallback = _callback;
	return *this;
}

UIElement& UIElement::SetHoverCallback(std::function<void(bool)> const& _callback)
{
	hoverCallback = _callback;
	return *this;
}

UIElement& UIElement::SetEnabled(bool enable) {
	isEnabled = enable;
	return *this;
}