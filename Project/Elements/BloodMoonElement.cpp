#include "BloodMoonElement.h"
#include "../Actor/Actor.h"
#include "Element.h"

void Elements::BloodMoonEffect(GameObject::CollisionData& target, Actor* caster)
{
	Actor* other{ dynamic_cast<Actor*>(&target.other) };
	if (!other || !caster) return;
	//Damage
	float dmg{};
	for (StatEffects::Mod const& m : Elements::bloodMoonDmgMods) {
		dmg += m.GetValFromActor(*caster);
	}
	other->TakeDamage(dmg, caster, DAMAGE_TYPE::ELEMENTAL);
	//Apply def shred debuff
	StatEffects::StatusEffect* debuff{ new StatEffects::StatusEffect{caster, Elements::bloodMoonDebuffDur, Elements::bloodMoonDebuffMaxStacks,
		Elements::bloodMoonDebuffName ,1, StatEffects::DEBUFF} };

	debuff->AddMod(Elements::bloodMoonDebuffMods);
	other->ApplyStatusEffect(debuff, caster);
}
