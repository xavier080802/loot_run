#ifndef _STATUS_EFFECT_H_
#define _STATUS_EFFECT_H_
#include <string>
#include <vector>
#include "../Actor/StatsTypes.h"

//Circular dependency
class Actor;

namespace Json {
	class Value;
}

namespace StatEffects {
	enum MATH_TYPE {
		FLAT, //Stat type doesnt matter, takes value as-is
		MULTIPLICATIVE, //Based on the stat's value
	};

	struct Mod {
		//If multiplicative, 1=1%
		float value;
		MATH_TYPE mathType;
		//Stat to affect
		STAT_TYPE stat;

		Mod(float _val, MATH_TYPE _mathType, STAT_TYPE _statToAffect) 
			: value(_val), mathType(_mathType), stat(_statToAffect){}

		Mod() : value{ 0 }, mathType{ FLAT }, stat{ STAT_TYPE::ATT } {}

		//Get this Mod's true value based on actor's base stats.
		//if mathType is MULT, returns value
		float GetValFromActor(Actor const& actor) const;

		//Object must contain "value"(float), "mathType"(0/1) and "stat"(int)
		static Mod ParseFromJSON(Json::Value const& v);
	};

	//Note: Don't change order, >= DEBUFF is considered debuff (Elements)
	enum EFF_TYPE {
		NONE,
		BUFF,
		DEBUFF,
		BLOOD, //Main blood effect
		SUN, //Main sun effect
		MOON, //Main moon effect
	};

	//Reason for SE to end
	enum END_REASON {
		TIMED_OUT,
		STACKS_ZERO,
		REMOVED,
	};

	//Status effect can have several stat buffs/debuffs
	class StatusEffect
	{
	public:
		//Set duration to -1 for no-timeout
		//Caster can be nullptr on construction. Will be set again when applied.
		StatusEffect(Actor* _caster, float _duration, unsigned _maxStacks, std::string _name, unsigned startStacks=1, EFF_TYPE _effType = EFF_TYPE::NONE, std::string _icon = "auto")
			: caster(_caster), duration(_duration), maxStacks(_maxStacks), durationTimer(0.f), name(_name),
			isPermanent(_duration == -1), effType(_effType), stacks(startStacks), icon{_icon} {

			//Set default icon based on type.
			if (_icon == "auto") {
				switch (_effType)
				{
				case StatEffects::DEBUFF:
					icon = "Assets/debuff.png";
					break;
				case StatEffects::BUFF:
					icon = "Assets/buff.png";
					break;
				case StatEffects::NONE:
				default:
					icon = "Assets/tiny.png";
					break;
				}
			}
		};

		virtual ~StatusEffect() {};

		//Add mod to the mods list of this SE. Can be chained.
		virtual StatusEffect* AddMod(Mod newMod);
		StatusEffect* AddMod(std::vector<Mod> mods);

		//Call when applying this effect to the entity (thereby referred to as owner)
		virtual void OnApply(Actor* _owner, Actor* _caster);

		//Update. Always call base (duration update)
		virtual void Tick(double dt);
		//Call when a new stack is to be applied. Increments stacks without needing a new SE
		virtual void OnReapply(int numStacks = 1);
		//Call when status effect ends (via timeout or removal). Func cleans up SE
		virtual void OnEnd(END_REASON reason = TIMED_OUT);

		//Get the value of all mods of a stat. Multiplicative is based on the baseVal.
		//Result will be an additive value
		virtual float GetFinalModVal(STAT_TYPE stat, float baseVal) const;

		virtual bool IsEnded() { return hasEnded; };

		std::string const& GetName() const { return name; };
		EFF_TYPE GetType() const { return effType; }
		std::string const& GetIcon() const { return icon; }
		std::vector<Mod>const& GetMods() const { return mods; }

		//Multiply the value of each Mod by the given scalar.
		void ScaleMods(float scalar);

		void SetIcon(std::string const& path);

	protected:
		Actor* caster{}, * owner{};
		//ID, should be unique.
		std::string name{};
		//Doesnt time out, but can be removed externally.
		bool isPermanent{ false };
		//Set in OnEnd
		bool hasEnded{ false };
		float duration{};
		float durationTimer{}; //to count down
		EFF_TYPE effType{};
		std::vector<Mod> mods{};
		//Filepath to icon
		std::string icon{};

		unsigned stacks{}, maxStacks{};
	};
}

#endif // !_STATUS_EFFECT_H_
