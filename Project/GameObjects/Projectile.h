#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_
#include "GameObject.h"

class Projectile : public GameObject
{
public:
	//Init proj and send it flying for [lifetime] seconds.
	//Inherits the caster's collision info (except size), z, and starting pos
	//Always call this before calling the other set-up functions
	Projectile* Fire(const GameObject* const caster, AEVec2 fireDir, float radius, float spd, float lifetime, void (*onHitCallback)(CollisionData& target));
	void Update(double dt) override;
	void OnCollide(CollisionData& other) override;

protected:
	AEVec2 dir{};
	const GameObject* owner{};
	float speed{};
	float lifespan{}; //seconds

	//Called when projectile hits an object that it can collide with.
	void (*OnHit)(CollisionData& target) {};
};
#endif // !_PROJECTILE_H_


