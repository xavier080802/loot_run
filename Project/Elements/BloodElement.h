#ifndef _BLOOD_ELEMENT_H_
#define _BLOOD_ELEMENT_H_
#include "Element.h"
#include "../Actor/ActorSubscriptions.h"

//DoT that counts as a hit.
//Reapplying triggers 1 instance of the DoT immediately
//Reapply as different name
class BloodElement : public StatEffects::StatusEffect
{
public:
	BloodElement(std::string _name) : StatEffects::StatusEffect{ nullptr, Elements::elementDur, 1, _name, 1, StatEffects::BLOOD }{}
	void Tick(double dt) override;
	void OnReapply(int numStacks = 1) override;

private:
	float tickCd{};

	void TriggerDoT();
};

#endif // !_BLOOD_ELEMENT_H_
