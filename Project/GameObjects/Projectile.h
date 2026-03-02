#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_
#include "GameObject.h"
#include "../Actor/Actor.h"
#include "../Elements/Element.h"

class Projectile : public GameObject
{
public:
	//Init proj and send it flying for [lifetime] seconds.
	//Inherits the caster's collision info (except size), z, and starting pos
	//Always call this before calling the other set-up functions
	Projectile* Fire(Actor* const caster, AEVec2 fireDir, float radius, float spd, float lifetime, void (*onHitCallback)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback), Elements::ELEMENT_TYPE elem = Elements::ELEMENT_TYPE::NONE, float kb = 100.0f);
	void Update(double dt) override;
	void OnCollide(CollisionData& other) override;
	void OnCollideTile(std::pair<TileMap::Tile const&, AEVec2> tile) override;

protected:
	AEVec2 dir{};
	Actor* owner{};
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE;
    float knockback = 100.0f;
	float speed{};
	float lifespan{}; //seconds

	//Called when projectile hits an object that it can collide with.
	void (*OnHit)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback) {};
};
#endif // !_PROJECTILE_H_


