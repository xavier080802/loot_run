#ifndef BLOODELEMENT_H_
#define BLOODELEMENT_H_

#include "Element.h"
#include "../Actor/ActorSubscriptions.h"

//DoT that should count as a hit.
//Reapplying triggers 1 instance of the DoT immediately
class BloodElement : public StatEffects::StatusEffect
{
public:
	BloodElement(std::string _name, StatEffects::EFF_TYPE _type = StatEffects::BLOOD, std::string const& icon = Elements::bloodIcon)
		: StatEffects::StatusEffect{ nullptr, Elements::elementDur, 1, _name, 1, _type, icon}{}
	virtual ~BloodElement() {}
	void Tick(double dt) override;
	void OnReapply(int numStacks = 1) override;

private:
	float tickCd{};

	virtual void TriggerDoT();
};

#endif // BLOODELEMENT_H_

