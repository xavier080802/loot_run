#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_
#include "GameObject.h"
#include "../Actor/Actor.h"
#include "../Elements/Element.h"
#include  <vector>

class Projectile : public GameObject
{
public:
	//Rule for when the proj deactivates
	enum class STOP_RULE {
		ON_FIRST, //Disabled on first object hit (default)
		FIRST_TILE, //Disabled on tile (goes through GOs)
		NONE, //Won't disable on-hit with anything
	};
	//Init proj and send it flying for [lifetime] seconds.
	//Inherits the caster's collision info (except size), z, and starting pos
	//Always call this before calling the other set-up functions
	Projectile* Fire(Actor* const caster, AEVec2 fireDir, float radius, float spd, float lifetime, void (*onHitCallback)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback), Elements::ELEMENT_TYPE elem = Elements::ELEMENT_TYPE::NONE, float kb = 100.0f);
	void Update(double dt) override;
	void OnCollide(CollisionData& other) override;
	void OnCollideTile(std::pair<TileMap::Tile const&, AEVec2> tile) override;

	//Set what will disable the projectile on-hit
	Projectile& SetStopRule(STOP_RULE rule = STOP_RULE::FIRST_TILE);

protected:
	AEVec2 dir{};
	Actor* owner{};
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE;
    float knockback = 100.0f;
	float speed{};
	float lifespan{}; //seconds
	STOP_RULE stopRule{ STOP_RULE::ON_FIRST };

	//Called when projectile hits an object that it can collide with.
	void (*OnHit)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback) {};

	std::vector<GameObject*> hasHitList{};
};
#endif // !_PROJECTILE_H_


