#include "BloodElement.h"
#include "../Actor/Actor.h"

void BloodElement::Tick(double dt)
{
	StatusEffect::Tick(dt);
	
	tickCd += static_cast<float>(dt);
	if (tickCd >= 1.f) {
		TriggerDoT();
		tickCd = 0.f;
	}
}

void BloodElement::TriggerDoT()
{
	//Damage calc

	//Do dmg in a way that calls the actor's ActorOnHitSubs

	owner->TakeDamage(1, caster, DAMAGE_TYPE::ELEMENTAL); //Temp
}

void BloodElement::OnReapply(int numStacks)
{
	StatusEffect::OnReapply(numStacks);
	TriggerDoT();
}
