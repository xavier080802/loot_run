#include "MoonElement.h"
#include "../Actor/Actor.h"
#include "../Inventory/EquipmentTypes.h"

MoonElement::~MoonElement()
{
	OnEnd(StatEffects::REMOVED);
}

void MoonElement::SubscriptionAlert(OnHitContent content)
{
	if (!content.attacker) return;
	//Heal attacker. If hit with melee, heal more
	float heal{};
	for (StatEffects::Mod const& m : Elements::healMods) {
		heal += m.GetValFromActor(*content.attacker);
	}
	content.attacker->Heal(heal * ((!content.weapon || content.weapon->isRanged) ? 1.f : Elements::moonMeleeHealMult) * stacks);
}

void MoonElement::SubscriptionAlert(ActorDeadSubContent content)
{
	if (!content.killer) return;
	//Heal killer by base heal amt + % of owner's max hp.
	float heal{};
	for (StatEffects::Mod const& m : Elements::healMods) {
		heal += m.GetValFromActor(*content.killer);
	}
	for (StatEffects::Mod const& m : Elements::moonKillHealMods) {
		heal += m.GetValFromActor(*owner);
	}
	content.killer->Heal(heal * stacks);
}

void MoonElement::OnApply(Actor* _owner, Actor* _caster)
{
	StatusEffect::OnApply(_owner, _caster);
	_owner->SubToOnHit(this);
	_owner->SubToOnDeath(this);

	//Add defense shred mod.
	AddMod(Elements::moonDebuffMods);
}

void MoonElement::OnEnd(StatEffects::END_REASON reason)
{
	if (hasEnded) return;
	StatusEffect::OnEnd(reason);
	if (owner) {
		owner->SubToOnHit(this, true);
		owner->SubToOnDeath(this, true);
	}
}
