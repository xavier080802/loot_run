#ifndef _PET_1_H
#define _PET_1_H
#include "Pet.h"

/*	Rock
	
	Skill:
	- Cast to start charging
	- Recast to yeet the rock
		Does more damage the longer the player charged for.
		Deals damage on-hit.
		At a certain charge time, also does AOE damage on-hit.
		Refunds cooldown based on inverse of charge time (Longer charge = less refund).

	NOTE: maxChargeDur is inclusive of aoeChargeThreshold 
		aoeChargeThreshold <= maxChargeDur < resetChargeDur
	NOTE 2: Player can only cast if the existing projectile is gone.
		Just say that the player yeets a metaphorical rock and they only have 1.

	Multipliers:
		[0] and [1] - Rock single target base hit damage (projectile)
		[2] and [3] - AOE damage (base)
		[4] and [5] - Max projectile damage
		[6] and [7] - Max AOE damage

	Extras:
		maxChargeDur - (float) Time till damage is maxed
		aoeChargeThreshold - (float) Time till damage hit becomes AOE. Should be < maxChargeDur
		resetChargeDur - (float) Time till skill cancels if player doesnt recast
		maxCDRefund - (float) How much (normalized) of the cooldown to refund, starts from this value down to 0 based on charge time.
		aoeDiameter - (float) Size of the AOE
		aoeCol - "r g b a" Colour of the AOE
		projRadius - (float) radius of the projectile
		projSpd - (float) Speed of the yeet
*/
class Pet_1 : public Pet
{
public:
	//Dmg of the non-aoe damage
	float CalcBasicDmg(Actor& caster);
	//Get charge mult [0,1] from charge time
	float GetChargeMult() const;
	//Charge mult[0,1] for AOE. 0 at aoeChargeThreshold, 1 at maxChargeDur
	float GetAOEChargeMult() const;
	//Call when proj hits something.
	void OnProjHit(Actor& caster, AEVec2 impactPos, bool allowAOE = true);

	bool ShowRecastUI() const override;
private:
	//Checks if can do AOE. Call when proj hits something
	void DoAOE(Actor& caster, AEVec2 impactPos);
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_1; }
	void Setup(Player& player) override;
	bool DoSkill(const Pets::SkillCastData& data) override;
	void SkillUpdate(float dt) override;

	/*	Implementation notes
		
		1. First cast: Start charging.
		2. Second cast: Fire proj.

		While proj is active, returned is false.
		There may be issue where proj can be fired faster than the prev one can poof
		(because the multiplier only applies on-hit, and its reset on each new cast),
		so only allow player to cast if the existing proj is gone.
		 - Proj is "gone" if it hits something (tile/GO) or timesout

		While charging, show some indicator (using world text rn) so player can
		have some idea of whats going on. At full power, show a specific text once.
	*/

	bool charging{ false }, returned{ true }, hasDoneChargeAlert{false};
	float maxChargeDur{}, resetChargeDur{}, chargeTimer{}, aoeChargeThreshold{},
		maxCDRefund{}, aoeDiameter{}, projRadius{}, projSpd{};
	Color aoeCol{};
};

#endif // !_PET_1_H
