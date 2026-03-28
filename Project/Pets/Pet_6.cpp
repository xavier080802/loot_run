#include "Pet_6.h"
#include "../Actor/Actor.h"
#include "../Actor/Player.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include "../Helpers/Vec2Utils.h"
#include "../Elements/Element.h"
#include "../GameObjects/GameObject.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "PetManager.h"
#include "../DebugTools.h"
#include <sstream>

namespace {
	void ProjHit(GameObject::CollisionData& other, Actor* caster, Elements::ELEMENT_TYPE element, float /*knockback*/) {
		if (!caster) return;

		if (other.other.GetGOType() != GO_TYPE::ENEMY) return;
		Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
		if (!pet) return;

		Actor& target = dynamic_cast<Actor&>(other.other);

		//Get multiplier from data and calc base damage
		caster->DealDamage(&target, pet->GetMultiplier(0).GetValFromActor(*caster) + pet->GetMultiplier(1).GetValFromActor(*caster),
			pet->GetPetData().dmgTypes.at(0), nullptr);

		//Apply element (if any)
		Elements::ApplyElement(pet->GetSkillElement(), caster, &target);
	}

	void SkillEffect(GameObject::CollisionData& other, Actor* caster, Elements::ELEMENT_TYPE /*element*/, float knockback, EquipmentData*, void* extra) {
		if (other.other.GetGOType() != GO_TYPE::ENEMY) return;
		Actor& target = dynamic_cast<Actor&>(other.other);
		
		// Knockback logic
		AEVec2 dir = {
			target.GetPos().x - caster->GetPos().x,
			target.GetPos().y - caster->GetPos().y
		};
		if (dir.x != 0.0f || dir.y != 0.0f) {
			AEVec2Normalize(&dir, &dir);
		}
		float knockbackForce = knockback;
		if (target.GetMaxHP() > 150.0f) {
			knockbackForce *= 0.4f;
		}
		target.ApplyForce({ dir.x * knockbackForce, dir.y * knockbackForce });

		//Deal damage
		Pets::PetData* petData{ static_cast<Pets::PetData*>(extra) };
		if (!petData) return;
		caster->DealDamage(&target, petData->multipliers.at(2).GetValFromActor(*caster) + petData->multipliers.at(3).GetValFromActor(*caster),
			petData->dmgTypes.at(0));
	}
}

void Pet_6::Setup(Player& p)
{
	knockback = std::stof(data.extra.at("knockback"));
	skillRange = std::stof(data.extra.at("skillRange"));
	attackCooldown = std::stof(data.extra.at("attackCooldown"));
	turretRange = std::stof(data.extra.at("turretRange"));
	projSpd = std::stof(data.extra.at("projSpd"));
	projSize = std::stof(data.extra.at("projSize"));
	projLife = std::stof(data.extra.at("projLife"));
	std::istringstream isc{ data.extra.at("projCol") };
	isc >> projCol.r >> projCol.g >> projCol.b >> projCol.a;
	std::istringstream isc2{ data.extra.at("skillCol") };
	isc2 >> skillCol.r >> skillCol.g >> skillCol.b >> skillCol.a;
	
	attackTimer = attackCooldown;
	player = &p;
}

bool Pet_6::DoSkill(const Pets::SkillCastData& _data)
{
	GameObject* go{ GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX) };
	AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(go) };
	if (!hb) return false;
	AttackHitboxConfig cfg{};
	cfg.knockback = knockback;
	cfg.colliderSize = cfg.renderScale = AEVec2{ skillRange, skillRange };
	cfg.followOwner = cfg.disableOnHit = false;
	cfg.tint = skillCol;
	cfg.owner = player;
	cfg.zIndex = -2;
	cfg.onHit = SkillEffect;
	cfg.extraData = &data;
	hb->Start(cfg);
	Bitmask bm{ cfg.owner->GetCollisionLayers() };
	hb->SetCollisionLayers(ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE));
	return true;
}

void Pet_6::SkillUpdate(float dt)
{
	attackTimer -= dt;

	if (attackTimer <= 0) {
		//Fire turret shot
		GameObject* go{ GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE) };
		Projectile* proj{ dynamic_cast<Projectile*>(go) };
		if (!proj) {
			Debug::stream << "PROJ FAILED TO FETCH\n";
			return;
		}
		//Find target
		GameObject* target{ GameObjectManager::GetInstance()->FindClosestGO(player->GetPos(), turretRange, GO_TYPE::ENEMY) };
		if (!target) {
			Debug::stream << "Dragon - No target found\n";
			return;
		}
		//Fire at target. Set color too
		proj->Fire(player, target->GetPos() - pos, projSize * 0.5f, projSpd, projLife, ProjHit,
			data.skillElements.empty() ? Elements::ELEMENT_TYPE::NONE : data.skillElements.at(0))
			->GetRenderData().tint = projCol;
		//Set pos to pet pos, not player pos
		proj->SetPos(pos);
		Bitmask bm{ proj->GetCollisionLayers() };
		ResetFlagAtPos(&bm, Collision::OBSTACLE);
		proj->SetCollisionLayers(bm);
		//Set stop rule so proj pierces
		proj->SetStopRule(Projectile::STOP_RULE::NONE);

		attackTimer = attackCooldown;
	}
}
