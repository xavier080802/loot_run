#ifndef _COLLISION_LAYER_LIST_H_
#define _COLLISION_LAYER_LIST_H_
namespace Collision {
	enum LAYER {
		NONE,
		PLAYER,
		ENEMIES,
		OBSTACLE,
		INTERACTABLE,
		PET,
	};

	enum SHAPE {
		COL_CIRCLE,
		COL_RECT,
	};
}
#endif // !_COLLISION_LAYER_LIST_H_

