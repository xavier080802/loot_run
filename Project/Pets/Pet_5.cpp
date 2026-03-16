#include "Pet_5.h"
#include "../Actor/Player.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "PetManager.h"
#include <string>
#include <sstream>

namespace {
	bool auraActive{false};

	void OnAuraHit(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float, void* extra)
	{
		Actor* other = dynamic_cast<Actor*>(&target.other);
		Pet_5* pet = static_cast<Pet_5*>(extra);
		if (!caster || !other || !pet) return;
		//Deal Damage
		float dmg{ pet->GetPetData().multipliers[0].GetValFromActor(*caster) + pet->GetPetData().multipliers[1].GetValFromActor(*caster)};
		caster->DealDamage(other, dmg, pet->GetPetData().dmgTypes[0]);
		//Apply element
		Elements::ApplyElement(element, caster, other);
		StatEffects::StatusEffect* sunSE{ other->GetStatusEffects().at(Elements::sunName) };
		if (sunSE && sunSE->GetStackCount()) {
			pet->IncSunCounter();
		}
	}

	void AuraEnd(Actor*) {
		auraActive = false;
	}
}

void Pet_5::IncSunCounter(unsigned count)
{
	if (!auraActive) return;

	sunCounter += count;

	if (sunCounter >= sunRequired) {
		sunCounter -= sunRequired;
		//Heal
		Player& p{ PetManager::GetInstance()->GetPlayer() };
		p.Heal(data.multipliers[2].GetValFromActor(p) + data.multipliers[3].GetValFromActor(p));
		//Cleanse 1 debuff
		for (auto& se : p.GetStatusEffects()) {
			if (se.second->GetType() != StatEffects::DEBUFF) continue;
			se.second->OnEnd(StatEffects::REMOVED);
			break;
		}
	}
}

void Pet_5::SubscriptionAlert(EffectAppliedContent content)
{
	IncSunCounter(content.stacksApplied);
}

void Pet_5::Setup(Player& player)
{
	auraSize = std::stof(data.extra["auraSize"]);
	auraDur = std::stof(data.extra["auraDuration"]);
	auraHitCd = std::stof(data.extra["auraTimeBetweenHits"]);
	sunRequired = std::stoi(data.extra["sunRequired"]);
	std::istringstream is{ data.extra["auraColor"] };
	is >> auraColor.r >> auraColor.g >> auraColor.b >> auraColor.a;

	//Cooldown is duration of aura + base cooldown
	data.skillCooldown += auraDur;
	auraActive = false;
	//Subscribe
	player.SubToSEGain(this);
}

bool Pet_5::DoSkill(const Pets::SkillCastData& _data)
{
	GameObject* go = GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX);
	AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(go);
	if (!hb) return false;

	//Set up the aura
	AttackHitboxConfig cfg{};
	cfg.owner = (Actor*)(&PetManager::GetInstance()->GetPlayer());
	cfg.colliderSize = cfg.renderScale = { auraSize, auraSize};
	cfg.tint = auraColor;
	cfg.followOwner = true;
	cfg.knockback = 0.f;
	cfg.lifetime = auraDur;
	cfg.disableOnHit = false;
	cfg.hitCooldown = auraHitCd;
	cfg.zIndex = -1;
	cfg.extraData = this;
	cfg.onHit = OnAuraHit;
	cfg.onEnd = AuraEnd;
	cfg.element = Elements::ELEMENT_TYPE::SUN;
	hb->Start(cfg);
	Bitmask bm{ cfg.owner->GetCollisionLayers() };
	hb->SetCollisionLayers(ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE));

	auraActive = true;
	sunCounter = 0;
	return true;
}


