#ifndef _UI_ELEMENT_H_
#define _UI_ELEMENT_H_
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include "../CollisionConstants.h"
#include <string>
#include <functional>

namespace Json {
	class Value;
}

/*
	Encapsulates callbacks for when the object is interacted with.

	Automatically registers itself with UIManager when contructed.
*/
class UIElement {
public:
	//Construct an element
	//Does not set callback functions
	//Registers to manager
	UIElement(AEVec2 _pos, AEVec2 _size, int _z, Collision::SHAPE _shape, bool _blockInteraction = true);

	//Construct an element from JSON
	//Does not set callback functions
	//Registers to manager
	UIElement(Json::Value const& json);

	void OnClick();
	void OnHover(bool mouseDown = false);
	void OnRelease();

	UIElement& SetClickCallback(std::function<void()>const& _callback);
	UIElement& SetReleaseCallback(std::function<void()>const& _callback);
	UIElement& SetHoverCallback(std::function<void(bool)>const& _callback);
	UIElement& SetEnabled(bool enable = true);

	int GetZ()const { return z; }
	Collision::SHAPE GetShape() const { return shape; }
	AEVec2 GetPos()const { return pos; }
	AEVec2 GetSize() const { return size; }
	bool DoesBlockInteraction() const { return blocksInteraction; }
	bool IsEnabled() const { return isEnabled; }

private:
	std::function<void()> clickCallback{}, releaseCallback{};
	std::function<void(bool)> hoverCallback{};
	int z{};
	AEVec2 pos{};
	AEVec2 size{};
	bool blocksInteraction{ true },
		isEnabled{ false };
	Collision::SHAPE shape{Collision::COL_RECT};
};

#endif // !_UI_ELEMENT_H_