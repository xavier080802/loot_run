#include "SunElement.h"
#include "../Actor/Actor.h"

SunElement::~SunElement()
{
	OnEnd(StatEffects::REMOVED);
}

void SunElement::SubscriptionAlert(OnHitContent content)
{
	if (hasEnded || !stacks || !content.attacker) return;
	//Transfer 1 stack to attacker
	--stacks;
	content.attacker->ApplyStatusEffect(CreateBuff(1), caster);
}

void SunElement::SubscriptionAlert(ActorDeadSubContent content)
{
	if (hasEnded || !stacks || !content.killer) return;
	//Transfer all stacks to killer
	content.killer->ApplyStatusEffect(CreateBuff(stacks), caster);
	stacks = 0;
}

void SunElement::OnApply(Actor* _owner, Actor* _caster)
{
	StatusEffect::OnApply(_owner, _caster);
	if (_owner) {
		_owner->SubToOnHit(this);
		_owner->SubToOnDeath(this);
	}
}

void SunElement::OnEnd(StatEffects::END_REASON reason)
{
	if (hasEnded) return;
	StatusEffect::OnEnd(reason);
	//Might get deleted before init
	if (owner) {
		owner->SubToOnHit(this, true);
		owner->SubToOnDeath(this, true);
	}
}

StatEffects::StatusEffect* SunElement::CreateBuff(unsigned numStacks)
{
	StatEffects::StatusEffect* se = new StatusEffect{caster, Elements::sunBuffDur, maxStacks, Elements::sunBuffName, numStacks, StatEffects::BUFF};
	se->AddMod(Elements::sunBuffMods);
	return se;
}
