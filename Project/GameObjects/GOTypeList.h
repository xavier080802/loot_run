#ifndef _GO_LIST_H
#define _GO_LIST_H

enum class GO_TYPE {
	NONE,
	PROJECTILE,
	ATTACK_HITBOX,
	ITEM,

	//NOTE: GOs below this comment are visible to minimap.
	PLAYER,
	LOOT_CHEST,
	ENEMY,

	//Pets
	PET_1,
	PET_2,
	PET_3,
	PET_4,
	PET_5,
	PET_6,

	NUM_GO_TYPES //Not a type. Last
};

// GoType >= this: Visible on minimap.
const GO_TYPE GO_TYPE_MINIMAP_RENDERABLE = GO_TYPE::PLAYER;
#endif // !_GO_LIST_H

