#include "BloodSunElement.h"
#include "../Actor/Actor.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../Helpers/Vec2Utils.h"

namespace {
	//On-death AOE dmg callback. Put here cuz otherwise hitbox onHit throws error.
	void AoEDmg(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE /*element*/, float /*knockback*/, void*extra)
	{
		Actor* other{ dynamic_cast<Actor*>(&target.other) };
		Actor* spawner{ static_cast<Actor*>(extra) }; //The SE owner on which this thing is detonating on.
		if (!other || !spawner) return;

		float dmg{};
		for (int i{}; i < Elements::bloodSunDetonateDmg.size(); ++i) {
			//0,1 - Scaling from spawner
			//2+ - Scaling from caster
			dmg += Elements::bloodSunDetonateDmg[i].GetValFromActor(i < 2 ? *spawner : *caster);
		}
		//Caster owns the dmg, not the spawner.
		caster->DealDamage(other, dmg, DAMAGE_TYPE::ELEMENTAL);
	}
}

void BloodSunElement::TriggerDoT()
{
	float dmg{};
	for (StatEffects::Mod const& m : Elements::bloodSunDotDmg) {
		dmg += m.GetValFromActor(*owner);
	}
	caster->DealDamage(owner, dmg, DAMAGE_TYPE::ELEMENTAL);
}

void BloodSunElement::SubscriptionAlert(ActorDeadSubContent content)
{
	//Dmg in an AOE
	AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)) };
	if (!hb) return;

	AttackHitboxConfig cfg{};
	cfg.owner = caster;
	cfg.offset = owner->GetPos() - caster->GetPos();
	cfg.colliderSize = cfg.renderScale = Elements::bloodSunDetoSize;
	cfg.followOwner = cfg.disableOnHit = false;
	cfg.extraData = owner;
	cfg.onHit = AoEDmg;
	cfg.tint = Elements::bloodSunDetoColor;
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
