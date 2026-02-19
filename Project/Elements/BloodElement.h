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
	BloodElement(std::string _name, StatEffects::EFF_TYPE _type = StatEffects::BLOOD) : StatEffects::StatusEffect{ nullptr, Elements::elementDur, 1, _name, 1, _type }{}
	virtual ~BloodElement() {}
	void Tick(double dt) override;
	void OnReapply(int numStacks = 1) override;

private:
	float tickCd{};

	virtual void TriggerDoT();
};

#endif // !_BLOOD_ELEMENT_H_
