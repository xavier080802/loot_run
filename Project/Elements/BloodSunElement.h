#ifndef BLOODSUNELEMENT_H_
#define BLOODSUNELEMENT_H_

#include "BloodElement.h"
#include "../Actor/ActorSubscriptions.h"

// Enhanced DoT effect.
// When the owner dies, does an instance of damage in an AOE
class BloodSunElement : public BloodElement, ActorDeadSub
{
public:
	BloodSunElement(std::string _name) : BloodElement{_name, StatEffects::DEBUFF, Elements::bloodSunIcon} {};
	//Remove subs from owner (if OnApply is called, owner should be non-null)
	~BloodSunElement();

	void OnApply(Actor* _owner, Actor* _caster) override;
	void OnEnd(StatEffects::END_REASON reason = StatEffects::TIMED_OUT) override;

	void SubscriptionAlert(ActorDeadSubContent content) override;

private:
	void TriggerDoT() override;
};

#endif // BLOODSUNELEMENT_H_

