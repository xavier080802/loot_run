#include "BloodSunElement.h"
#include "../Actor/Actor.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/AttackHitboxGO.h"

namespace {
	//On-death AOE dmg callback. Put here cuz otherwise hitbox onHit throws error.
	//caster is the status effect owner
	void AoEDmg(GameObject::CollisionData& target, Actor* caster)
	{
		Actor* other{ dynamic_cast<Actor*>(&target.other) };
		if (!other) return;

		float dmg{};
		for (StatEffects::Mod const& m : Elements::bloodSunDetonateDmg) {
			dmg += m.GetValFromActor(*caster);
		}
		other->TakeDamage(dmg, caster, DAMAGE_TYPE::ELEMENTAL);
	}
}

void BloodSunElement::TriggerDoT()
{
	float dmg{};
	for (StatEffects::Mod const& m : Elements::bloodSunDotDmg) {
		dmg += m.GetValFromActor(*owner);
	}
	owner->TakeDamage(dmg, caster, DAMAGE_TYPE::ELEMENTAL);
}

void BloodSunElement::SubscriptionAlert(ActorDeadSubContent content)
{
	//Dmg in an AOE
	AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)) };
	if (!hb) return;

	AttackHitboxConfig cfg{};
	cfg.owner = owner;
	cfg.colliderSize = cfg.renderScale = Elements::bloodSunDetoSize;
	cfg.followOwner = cfg.disableOnHit = false;
	cfg.onHit = AoEDmg;
	hb->Start(cfg);
	//Change collision layer from owner-of-SE to caster
	hb->SetCollisionLayers(caster->GetCollisionLayers());
	hb->SetColliderLayer(caster->GetColliderLayer());
}

BloodSunElement::~BloodSunElement()
{
	OnEnd(StatEffects::REMOVED);
}

void BloodSunElement::OnApply(Actor* _owner, Actor* _caster)
{
	BloodElement::OnApply(_owner, _caster);
	_owner->SubToOnDeath(this);
}

void BloodSunElement::OnEnd(StatEffects::END_REASON reason)
{
	if (hasEnded) return;
	BloodElement::OnEnd(reason);
	if (owner) {
		owner->SubToOnDeath(this, true);
	}
}
