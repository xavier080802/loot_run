#include "AEMath.h"
#include "StatusEffect.h"
#include "Actor.h"

StatEffects::StatusEffect* StatEffects::StatusEffect::AddMod(Mod newMod)
{
	mods.push_back(newMod);
	return this;
}

void StatEffects::StatusEffect::OnApply(Actor* _owner, Actor* _caster)
{
	owner = _owner;
	caster = _caster;
}

void StatEffects::StatusEffect::Tick(double dt)
{
	durationTimer += static_cast<float>(dt);

	if (durationTimer >= duration || !stacks) {
		OnEnd(stacks ? END_REASON::TIMED_OUT : END_REASON::STACKS_ZERO);
	}
}

void StatEffects::StatusEffect::OnEnd(END_REASON reason)
{
	(void)reason; //Unused param
	hasEnded = true;
}

void StatEffects::StatusEffect::OnReapply(int numStacks)
{
	if (stacks < maxStacks) {
		//Add stacks up to the cap.
		stacks += numStacks;
		if (stacks > maxStacks) stacks = maxStacks;
	}

	//Refresh timer
	durationTimer = 0.f;
}

float StatEffects::StatusEffect::GetFinalModVal(STAT_TYPE stat, float baseVal) const
{
	float change{};
	for (Mod m : mods) {
		if (m.stat != stat) continue;
		change += (m.mathType == FLAT ? m.value : baseVal * (m.value / 100.f));
	}
	return change * stacks;
}

void StatEffects::StatusEffect::ScaleMods(float scalar)
{
	for (Mod& m : mods) {
		m.value *= scalar;
	}
}

float StatEffects::Mod::GetValFromActor(Actor const& actor) const
{
	if (mathType == FLAT) return value;

	const ActorStats& stats{ actor.GetBaseStats() };
	float out{};
	switch (stat)
	{
	case MAX_HP:
		out = stats.maxHP;
		break;
	case DEF:
		out = stats.defense;
		break;
	case ATT:
		out = stats.attack;
		break;
	case ATT_SPD:
		out = stats.attackSpeed;
		break;
	case MOVE_SPD:
		out = stats.moveSpeed;
		break;
	default:
		break;
	}

	return out * (value / 100.f);
}
