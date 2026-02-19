#include "MoonElement.h"
#include "../Actor/Actor.h"
#include "../Inventory/EquipmentTypes.h"

MoonElement::~MoonElement()
{
	OnEnd(StatEffects::REMOVED);
}

void MoonElement::SubscriptionAlert(OnHitContent content)
{
	//Heal attacker. If hit with melee, heal more
	content.attacker->Heal(((!content.weapon || content.weapon->isRanged) ? 5.f : 7.5f) * stacks);
}

void MoonElement::SubscriptionAlert(ActorDeadSubContent content)
{
	//Heal killer by % of owner's max hp.
	content.killer->Heal(owner->GetMaxHP() * 2.5f * stacks);
}

void MoonElement::OnApply(Actor* _owner, Actor* _caster)
{
	StatusEffect::OnApply(_owner, _caster);
	_owner->SubToOnHit(this);
	_owner->SubToOnDeath(this);

	//Add defense shred mod.
	AddMod({ 5, StatEffects::MATH_TYPE::MULTIPLICATIVE, STAT_TYPE::DEF });
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
