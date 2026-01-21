#ifndef _STATUS_EFFECT_H_
#define _STATUS_EFFECT_H_
#include <string>
#include <vector>

//TEMP
enum STAT_TYPE {
	TEST_ATT,
};

namespace StatEffects {
	enum MATH_TYPE {
		FLAT,
		MULTIPLICATIVE,
	};

	struct Mod {
		//If multiplicative, 1=1%
		float value;
		MATH_TYPE mathType;
		//Stat to affect
		STAT_TYPE stat;
	};

	//Status effect can have several stat buffs/debuffs
	class StatusEffect
	{
	public:
		//TODO: include the entity that casted.
		//Set duration to -1 for no-timeout
		StatusEffect(float _duration, unsigned _maxStacks)
			: duration(_duration), maxStacks(_maxStacks), durationTimer(0.f), isPermanent(_duration == -1) {};

		//Add mod to the mods list of this SE. Can be chained.
		virtual StatusEffect* AddMod(Mod newMod);

		//Call when applying this effect to the entity (thereby referred to as owner)
		virtual void OnApply(int tempPlsPutEntityClass_owner, int tempPlsPutEntityClass_Caster);

		//Update. Always call base (duration update)
		virtual void Tick(double dt);
		//Call when a new stack is to be applied.
		virtual void OnReapply(int numStacks = 1);
		//Get the value of all mods of a stat. Multiplicative is based on the baseVal.
		//Result will be an additive value
		virtual float GetFinalModVal(STAT_TYPE stat, float baseVal) const;

	protected:
		//Doesnt time out, but can be removed externally.
		bool isPermanent{ false };
		float duration{};
		float durationTimer{}; //to count down

		std::vector<Mod> mods{};

		unsigned stacks{}, maxStacks{};

		//Called when status effect duration ends
		virtual void OnEnd();
	};
}

#endif // !_STATUS_EFFECT_H_
