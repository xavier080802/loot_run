#include "SunMoonElement.h"
#include "Element.h"
#include "../Actor/Actor.h"
#include "../GameObjects/AttackHitboxGO.h"
#include <map>

namespace {
	void DetonateDoDmg(GameObject::CollisionData& target, Actor* caster) {
		Actor* other{ dynamic_cast<Actor*>(&target.other) };
		if (!other || !caster) return;

		float dmg{};
		for (StatEffects::Mod const& m : Elements::sunMoonDmgMods) {
			dmg += m.GetValFromActor(*caster);
		}
		other->TakeDamage({ dmg, caster, DAMAGE_TYPE::ELEMENTAL, nullptr });
	}
}

namespace Elements {
	void SunMoonEffect(GameObject::CollisionData& target, Actor* caster)
	{
		Actor* other{ dynamic_cast<Actor*>(&target.other) };
		if (!other) return;

		std::map<std::string, StatEffects::StatusEffect*> const& dict{ other->GetStatusEffects() };

		//For owner...
		if (other == caster) {
			//Increase buff stacks on caster(owner)
			for (auto& pair : dict) {
				if (pair.second->GetType() >= StatEffects::DEBUFF) continue; //Skip debuffs
				pair.second->OnReapply(1);
			}
			return;
		}
	
		//For other actors...
		
		//Increase debuff stacks
		for (auto& pair : dict) {
			if (pair.second->GetType() < StatEffects::DEBUFF) continue; //Skip buffs
			pair.second->OnReapply(1);
		}

		//Apply Slow
		StatEffects::StatusEffect* slow{ new StatEffects::StatusEffect{caster, sunMoonDebuffDur, sunMoonDebuffMaxStacks, sunMoonDebuffName, 1, StatEffects::DEBUFF } };
		slow->AddMod(sunMoonDebuffMods);
		other->ApplyStatusEffect(slow, caster);
	}

	void SunMoonDetonate(Actor* caster)
	{
		//Do Damage
		AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)) };
		if (!hb) return;

		AttackHitboxConfig cfg{ };
		cfg.owner = caster;
		cfg.colliderSize = cfg.renderScale = sunMoonSize;
		cfg.disableOnHit = cfg.followOwner= false;
		cfg.tint = { 0,0,0,150 };
		cfg.onHit = DetonateDoDmg;
		hb->Start(cfg);
	}
}

