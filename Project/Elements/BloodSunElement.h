#ifndef _BLOOD_SUN_ELEMENT_H_
#define _BLOOD_SUN_ELEMENT_H_
#include "BloodElement.h"
#include "../Actor/ActorSubscriptions.h"

class BloodSunElement : public BloodElement, ActorDeadSub
{
public:
	BloodSunElement(std::string _name) : BloodElement{_name, StatEffects::DEBUFF} {};
	//Remove subs from owner (if OnApply is called, owner should be non-null)
	~BloodSunElement();

	void OnApply(Actor* _owner, Actor* _caster) override;
	void OnEnd(StatEffects::END_REASON reason = StatEffects::TIMED_OUT) override;

	void SubscriptionAlert(ActorDeadSubContent content) override;

private:
	void TriggerDoT() override;
};

#endif // !_BLOOD_SUN_ELEMENT_H_
