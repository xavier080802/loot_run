#include "Projectile.h"
#include <iostream>

Projectile* Projectile::Fire(const GameObject* const caster, AEVec2 fireDir, float radius, float spd, float lifetime, void (*onHitCallback)(CollisionData& target))
{
	Init(caster->GetPos(), { radius * 2.f, radius * 2.f }, caster->GetZ(),
		MESH_CIRCLE, COL_CIRCLE, { radius * 2.f, radius * 2.f },
		caster->GetCollisionLayers(), caster->GetColliderLayer());

	owner = caster;
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
	lifespan -= dt;
	//Move in dir
	pos.x += dir.x * speed * dt;
	pos.y += dir.y * speed * dt;

	//Check timeout
	if (lifespan <= 0.f) {
		isEnabled = false; //Grew old and died.
	}
}

void Projectile::OnCollide(CollisionData& other)
{
	//Disable self
	isEnabled = false;
	//Send callback
	if (OnHit) OnHit(other);
}
