#include "SunElement.h"
#include "../Actor/Actor.h"

void SunElement::SubscriptionAlert(OnHitContent content)
{
	if (hasEnded || !stacks) return;
	//Transfer 1 stack to attacker
	--stacks;
	content.attacker->ApplyStatusEffect(CreateBuff(1), caster);
}

void SunElement::SubscriptionAlert(ActorDeadSubContent content)
{
	if (hasEnded || !stacks) return;
	//Transfer all stacks to killer
	content.killer->ApplyStatusEffect(CreateBuff(stacks), caster);
	stacks = 0;
}

void SunElement::OnApply(Actor* _owner, Actor* _caster)
{
	StatusEffect::OnApply(_owner, _caster);
	_owner->SubToOnHit(this);
	_owner->SubToOnDeath(this);
}

void SunElement::OnEnd(END_REASON reason)
{
	StatusEffect::OnEnd(reason);
	owner->SubToOnHit(this, true);
	owner->SubToOnDeath(this, true);
}

StatEffects::StatusEffect* SunElement::CreateBuff(unsigned numStacks)
{
	//TODO: stat mods
	return new StatusEffect{caster, Elements::sunBuffDur, maxStacks, Elements::sunBuffName, numStacks, StatEffects::BUFF};
}
