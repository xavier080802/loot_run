#include "BloodMoonElement.h"
#include "../Actor/Actor.h"
#include "Element.h"

void Elements::BloodMoonEffect(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE /*element*/, float /*knockback*/)
{
	Actor* other{ dynamic_cast<Actor*>(&target.other) };
	if (!other) return;
	//Damage
	other->TakeDamage(5, caster, DAMAGE_TYPE::ELEMENTAL);
	//Apply def shred debuff
	StatEffects::StatusEffect* debuff{ new StatEffects::StatusEffect{caster, Elements::bloodMoonDebuffDur, Elements::bloodMoonDebuffMaxStacks,
		Elements::bloodMoonDebuffName ,1, StatEffects::DEBUFF} };

	debuff->AddMod({ -5, StatEffects::MATH_TYPE::MULTIPLICATIVE, STAT_TYPE::DEF });
	other->ApplyStatusEffect(debuff, caster);
}
