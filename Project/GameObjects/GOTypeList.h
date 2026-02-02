#ifndef _GO_LIST_H
#define _GO_LIST_H

enum class GO_TYPE {
	NONE,
	PLAYER,
	ENEMY,
	PROJECTILE,
	ATTACK_HITBOX,
	LOOT_CHEST,
	ITEM, //Temp? for LootChest

	NUM_GO_TYPES //Last
};
#endif // !_GO_LIST_H

