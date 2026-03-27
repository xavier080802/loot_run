#ifndef _PET_4_
#define _PET_4_
#include "Pet.h"
#include "../Actor/Player.h"
#include "../Helpers/ColorUtils.h"
class Whirlpool;

/*	Scylla
	
	Forms a whirlpool at the mouse location, pulling enemies towards its center.
	Press skill key while the pool is active to end it early.
	When pool ends, damages enemies inside it.
	Skill cooldown actually starts when pool dies.

	extra:
		pull: Pull foce applied each frame (with dt)
		duration: Duration of the whirlpool
		size: Size of the whirlpool, for both x and y values
		color: "r g b a" for color of the pool
		minDmgFalloff: float multiplier for the distance-based dmg falloff

	multipliers:
		[0] and [1] - On-end damage
		
	1 element required
*/
class Pet_4 : public Pet
{
public:
	void Setup(Player& player) override;

	Whirlpool* GetWhirlpool() { return pool; }

	float GetMinDmgMult() { return minDmgMult; }

	bool ShowRecastUI() const override;

private:
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_4; }

	bool DoSkill(const Pets::SkillCastData& _data) override;
	void SkillUpdate(float dt) override;

	bool Recast();

	float pullStrength{}, duration{}, minDmgMult{};
	float recastTimer{};
	AEVec2 poolSize{};
	Whirlpool* pool{};
	Color poolCol{};
	bool poolActive{};
};
#endif

