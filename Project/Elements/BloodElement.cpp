#include "BloodElement.h"
#include "../Actor/Actor.h"
using namespace Elements;

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

	float dmg{};
	for (StatEffects::Mod const& m : bloodDmgMods) {
		dmg += m.GetValFromActor(*caster);
	}
	owner->TakeDamage(dmg, caster, DAMAGE_TYPE::ELEMENTAL);
}

void BloodElement::OnReapply(int numStacks)
{
	StatusEffect::OnReapply(numStacks);
	TriggerDoT();
}
