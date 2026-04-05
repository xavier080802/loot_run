#ifndef MOONELEMENT_H_
#define MOONELEMENT_H_

#include "Element.h"
#include "../Actor/ActorSubscriptions.h"

//Reduces def of owner
//When hit, attacker heals. They heal more if they are melee
//When killed, killer heals based on an additional % owner's max hp
class MoonElement : public StatEffects::StatusEffect, ActorOnHitSub, ActorDeadSub
{
public:
	MoonElement() : StatEffects::StatusEffect{ nullptr, Elements::elementDur, Elements::maxMoonStacks, Elements::moonName, 1, StatEffects::MOON, Elements::moonIcon}{}
	//Remove subs from owner (if OnApply is called, owner should be non-null)
	~MoonElement();

	void SubscriptionAlert(OnHitContent content) override;
	void SubscriptionAlert(ActorDeadSubContent content) override;

	void OnApply(Actor* _owner, Actor* _caster) override;
	void OnEnd(StatEffects::END_REASON reason = StatEffects::END_REASON::TIMED_OUT) override;

private:
};

#endif // MOONELEMENT_H_

