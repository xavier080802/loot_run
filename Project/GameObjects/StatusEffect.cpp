#include "StatusEffect.h"
#include "AEMath.h"

StatEffects::StatusEffect* StatEffects::StatusEffect::AddMod(Mod newMod)
{
	mods.push_back(newMod);
	return this;
}

void StatEffects::StatusEffect::OnApply(int tempPlsPutEntityClass_owner, int tempPlsPutEntityClass_Caster)
{
	//TODO: assign owner ent and caster ent.

}

void StatEffects::StatusEffect::Tick(double dt)
{
	durationTimer += static_cast<float>(dt);

	if (durationTimer >= duration) {
		OnEnd();
	}
}

void StatEffects::StatusEffect::OnEnd()
{
	//Remove from owner

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
	return change;
}
