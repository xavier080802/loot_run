#include "Whirlpool.h"
#include "../Actor/Actor.h"
#include "../Helpers/Vec2Utils.h"

void Whirlpool::Update(double dt)
{
	GameObject::Update(dt);

	deltaTime = (float)dt;
	timer -= deltaTime;
	if (timer <= 0) {
		SetEnabled(false);
	}
}

void Whirlpool::OnCollide(CollisionData& other)
{
	//Check for actor
	Actor* a{ dynamic_cast<Actor*>(&other.other) };
	if (!a) return;
	//Pull in
	a->ApplyForce((pos - a->GetPos()) * strength * deltaTime);
}

//Init
Whirlpool& Whirlpool::SetupWhirlpool(float _duration, float _strength)
{
	duration = _duration;
	strength = _strength;
	timer = duration;
	return *this;
}
