#ifndef _SUN_ELEMENT_H_
#define _SUN_ELEMENT_H_
#include "Element.h"
#include "../Actor/ActorSubscriptions.h"

//When owner is hit, remove 1 stack and give 1 stack of [buff] to the attacker
//If owner dies, all remaining [buff] stacks are given to the killer.
class SunElement : public StatEffects::StatusEffect, ActorOnHitSub, ActorDeadSub
{
public:
	SunElement(unsigned _stacks) 
		: StatusEffect{ nullptr, Elements::elementDur, Elements::maxSunStacks, Elements::sunName, _stacks, StatEffects::SUN } {};

	void SubscriptionAlert(OnHitContent content) override;
	void SubscriptionAlert(ActorDeadSubContent content) override;

	void OnApply(Actor* _owner, Actor* _caster) override;
	void OnEnd(END_REASON reason = TIMED_OUT) override;

private:
	//The buff that sun stacks convert to when attacker hits this actor
	StatusEffect* CreateBuff(unsigned numStacks);
};

#endif // !_SUN_ELEMENT_H_
