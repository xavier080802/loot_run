#ifndef _PET_5_H
#define _PET_5_H
#include "Pet.h"
#include "../Helpers/ColorUtils.h"
#include "../Actor/ActorSubscriptions.h"

/*	Pheonix
	
	Skill: 
		Create aura that follows the player.
		Periodically, the aura applies Sun then damages enemies.
		If there is a sun element on the enemy after the damage, increments a counter.
		Once counter reaches certain amount, player is healed and a debuff is cleansed,
		then counter resets.
		Includes sun buff stacks obtained from killing afflicted enemies while the aura is up.

	Multipliers: 
		[0] and [1] for aura damage
		[2] and [3] for healing

	extra:
		auraDuration - float for how long the aura lasts
		auraSize - float for the aura collider size
		auraTimeBetweenHits - float per time between aura ticks
		auraColor - Tint of the aura
		sunRequired - Sun stacks required (hits on sun-afflicted enemies) to trigger heal
*/
class Pet_5 : public Pet, ActorGainedStatusEffectSub
{
public:
	void IncSunCounter(unsigned count = 1);

	void SubscriptionAlert(EffectAppliedContent content) override;

private:
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_5; }

	void Setup(Player& player) override;

	bool DoSkill(const Pets::SkillCastData& _data) override;

	//Aura stuff
	unsigned sunCounter{}, sunRequired{5};

	//Cache from json extras

	float auraSize{}, auraDur{}, auraHitCd{};
	Color auraColor{};
};

#endif // !_PET_5_H
