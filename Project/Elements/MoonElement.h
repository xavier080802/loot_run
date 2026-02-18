#ifndef _MOON_ELEMENT_H_
#define _MOON_ELEMENT_H_
#include "Element.h"
#include "../Actor/ActorSubscriptions.h"

//Reduces def of owner
//When hit, attacker heals. They heal more if they are melee
//When killed, killer heals based on a % owner's max hp
class MoonElement : public StatEffects::StatusEffect, ActorOnHitSub, ActorDeadSub
{
public:
	MoonElement() : StatEffects::StatusEffect{ nullptr, Elements::elementDur, Elements::maxMoonStacks, Elements::moonName, 1, StatEffects::MOON }{}

	void SubscriptionAlert(OnHitContent content) override;
	void SubscriptionAlert(ActorDeadSubContent content) override;

	void OnApply(Actor* _owner, Actor* _caster) override;
	void OnEnd(END_REASON reason = TIMED_OUT) override;

private:
	
};

#endif // !_MOON_ELEMENT_H_
