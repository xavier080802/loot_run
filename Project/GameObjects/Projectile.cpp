#include "Projectile.h"
#include "../TileMap.h"
#include <iostream>

Projectile* Projectile::Fire(Actor* const caster, AEVec2 fireDir, float radius, float spd, float lifetime, void (*onHitCallback)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback), Elements::ELEMENT_TYPE elem, float kb)
{
	hasHitList.clear();
	Init(caster->GetPos(), { radius * 2.f, radius * 2.f }, caster->GetZ(),
		MESH_CIRCLE, Collision::COL_CIRCLE, { radius * 2.f, radius * 2.f },
		caster->GetCollisionLayers(), caster->GetColliderLayer());

	owner = caster;
    element = elem;
    knockback = kb;
	AEVec2Normalize(&dir, &fireDir);
	speed = spd;
	lifespan = lifetime;
	goType = GO_TYPE::PROJECTILE;
	OnHit = onHitCallback;
	return this;
}

void Projectile::Update(double dt)
{
	//Tick down lifespan
	lifespan -= static_cast<float>(dt);
	//Move in dir
	pos.x += dir.x * speed * static_cast<float>(dt);
	pos.y += dir.y * speed * static_cast<float>(dt);

	//Check timeout
	if (lifespan <= 0.f) {
		isEnabled = false; //Grew old and died.
	}
}

void Projectile::OnCollide(CollisionData& other)
{
	//Stop rule makes it so proj wont disable on hit with GO
	if (stopRule != STOP_RULE::ON_FIRST) {
		//Check if has hit this go before
		for (GameObject* go : hasHitList) {
			if (go == &other.other) {
				return; //Yes, don't hit again
			}
		}
		//No, continue but add to list
		hasHitList.push_back(&other.other);
	}
	//Send callback
	if (OnHit) OnHit(other, owner, element, knockback);
	//Disable self if stop rule says so
	if (stopRule == STOP_RULE::ON_FIRST) {
		isEnabled = false;
	}
}

void Projectile::OnCollideTile(std::pair<TileMap::Tile const&, AEVec2> /*tile*/)
{
	if (stopRule == STOP_RULE::ON_FIRST || stopRule == STOP_RULE::FIRST_TILE) {
		return;
	}
	//Disable self
	isEnabled = false;
}

Projectile& Projectile::SetStopRule(STOP_RULE rule)
{
	hasHitList.reserve(10);
	stopRule = rule;
	return *this;
}
