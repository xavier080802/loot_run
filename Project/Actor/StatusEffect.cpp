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
	if (stacks == maxStacks) return;

	//Add stacks up to the cap.
	stacks += numStacks;
	if (stacks > maxStacks) stacks = maxStacks;

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
