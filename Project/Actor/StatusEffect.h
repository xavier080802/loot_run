#ifndef _STATUS_EFFECT_H_
#define _STATUS_EFFECT_H_
#include <string>
#include <vector>
#include "../Actor/StatsTypes.h"

//Circular dependency
class Actor;

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

		Mod(float _val, MATH_TYPE _mathType, STAT_TYPE _statToAffect) 
			: value(_val), mathType(_mathType), stat(_statToAffect){}
	};

	enum EFF_TYPE {
		NONE,
		BUFF,
		DEBUFF
	};

	//Status effect can have several stat buffs/debuffs
	class StatusEffect
	{
	public:
		//Set duration to -1 for no-timeout
		//Caster can be nullptr on construction. Will be set again when applied.
		StatusEffect(Actor* _caster, float _duration, unsigned _maxStacks, std::string _name, unsigned startStacks=1, EFF_TYPE _effType = EFF_TYPE::NONE)
			: caster(_caster), duration(_duration), maxStacks(_maxStacks), durationTimer(0.f), name(_name),
			isPermanent(_duration == -1), effType(_effType), stacks(startStacks){};

		//Add mod to the mods list of this SE. Can be chained.
		virtual StatusEffect* AddMod(Mod newMod);

		//Call when applying this effect to the entity (thereby referred to as owner)
		virtual void OnApply(Actor* _owner, Actor* _caster);

		//Update. Always call base (duration update)
		virtual void Tick(double dt);
		//Call when a new stack is to be applied.
		virtual void OnReapply(int numStacks = 1);
		//Get the value of all mods of a stat. Multiplicative is based on the baseVal.
		//Result will be an additive value
		virtual float GetFinalModVal(STAT_TYPE stat, float baseVal) const;

		virtual bool IsEnded() { return hasEnded; };

		std::string const& GetName() const { return name; };
		EFF_TYPE GetType() const { return effType; }

	protected:
		Actor* caster{}, * owner{};
		//ID, should be unique.
		std::string name{};
		//Doesnt time out, but can be removed externally.
		bool isPermanent{ false };
		bool hasEnded{ false };
		float duration{};
		float durationTimer{}; //to count down
		EFF_TYPE effType{};
		std::vector<Mod> mods{};

		unsigned stacks{}, maxStacks{};

		//Reason for SE to end
		enum END_REASON {
			TIMED_OUT,
			STACKS_ZERO,
			REMOVED,
		};

		//Called when status effect ends (via timeout or removal)
		virtual void OnEnd(END_REASON reason = TIMED_OUT);
	};
}

#endif // !_STATUS_EFFECT_H_
