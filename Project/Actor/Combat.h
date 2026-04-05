#ifndef COMBAT_H_
#define COMBAT_H_

#include <AEEngine.h>
#include "../Actor/Actor.h"
#include "../Inventory/EquipmentTypes.h"
#include "../GameObjects/GameObject.h"
#include "../Elements/Element.h"

namespace Combat
{
	// Executes an attack at the caster's position using the specified weapon attributes.
	// Can be used by both Players and Enemies to spawn projectiles or melee hitboxes.
	// targetPos: World coordinates indicating where the caster is aiming (e.g. mouse for player, target player for enemy).
	void ExecuteAttack(Actor* caster, const EquipmentData* weapon, AEVec2 targetPos);

	// Executes an attack using the specified weapon attributes.
	// Can be used by both Players and Enemies to spawn projectiles or melee hitboxes.
	// targetPos: World coordinates indicating where the caster is aiming (e.g. mouse for player, target player for enemy).
	void ExecuteAttack(Actor* caster, AEVec2 casterPos, const EquipmentData* weapon, AEVec2 targetPos);

	void OnProjectileHit(GameObject::CollisionData& data, Actor* caster, Elements::ELEMENT_TYPE element, float knockback);

	void OnMeleeHit(GameObject::CollisionData& data, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, void* ex =nullptr);
}

#endif // COMBAT_H_

