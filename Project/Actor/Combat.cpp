#include "Combat.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Elements/Element.h"
#include <iostream>

namespace Combat
{
	// Used by Projectile collisions
	void OnProjectileHit(GameObject::CollisionData& data, Actor* caster, Elements::ELEMENT_TYPE element, float /*knockback*/)
	{
		if (!caster) return;

        // Player hits ENEMY. Enemy hits PLAYER.
		GO_TYPE casterType = caster->GetGOType();
		GO_TYPE targetType = data.other.GetGOType();

		if (casterType == GO_TYPE::PLAYER && targetType != GO_TYPE::ENEMY) return;
		if (casterType == GO_TYPE::ENEMY && targetType != GO_TYPE::PLAYER) return;

		Actor& target = static_cast<Actor&>(data.other);

		if (target.IsInvulnerable()) return;

		// Calculate base damage from stats
		caster->DealDamage(&target, caster->GetStats().attack, DAMAGE_TYPE::PHYSICAL, nullptr);

        if (element != Elements::ELEMENT_TYPE::NONE) {
            std::cout << "[Combat] Projectile hit applied element: " << static_cast<int>(element) << "\n";
            Elements::ApplyElement(element, caster, &target);
        }
	}

	// Used by AttackHitboxGO collisions
	void OnMeleeHit(GameObject::CollisionData& data, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, void*)
	{
		if (!caster) return;
        
        GO_TYPE casterType = caster->GetGOType();
		GO_TYPE targetType = data.other.GetGOType();

		if (casterType == GO_TYPE::PLAYER && targetType != GO_TYPE::ENEMY) return;
		if (casterType == GO_TYPE::ENEMY && targetType != GO_TYPE::PLAYER) return;

		Actor& target = static_cast<Actor&>(data.other);

		if (target.IsInvulnerable()) return;

		caster->DealDamage(&target, caster->GetStats().attack, DAMAGE_TYPE::PHYSICAL, nullptr);

        if (element != Elements::ELEMENT_TYPE::NONE) {
            std::cout << "[Combat] Melee hit applied element: " << static_cast<int>(element) << "\n";
            Elements::ApplyElement(element, caster, &target);
        }

		// Knockback logic
		AEVec2 dir = {
		    target.GetPos().x - caster->GetPos().x,
		    target.GetPos().y - caster->GetPos().y
		};

		if (dir.x != 0.0f || dir.y != 0.0f) {
			AEVec2Normalize(&dir, &dir);
        }

		float knockbackForce = knockback;
		if (target.GetMaxHP() > 200.0f) {
			knockbackForce *= 0.4f;
		}

		target.ApplyForce({ dir.x * knockbackForce, dir.y * knockbackForce });
	}


	void ExecuteAttack(Actor* caster, const EquipmentData* weapon, AEVec2 targetPos)
	{
		if (!caster || !weapon) return;

        AEVec2 pos = caster->GetPos();

		switch (weapon->attackType)
		{
		case AttackType::Projectile:
		{
			Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
			if (!proj) return;

			AEVec2 fireDir = { targetPos.x - pos.x, targetPos.y - pos.y };

			// Fire(caster, direction, radius, speed, lifetime, callback, element, knockback)
			proj->Fire(caster, fireDir, weapon->attackSize, 200.0f, 3.0f, &OnProjectileHit, weapon->element, weapon->knockback);
			Bitmask bm{ caster->GetCollisionLayers() };
			ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
			proj->SetCollisionLayers(bm); // Proj will scan for interactables otherwise
			
			std::cout << "[Combat] Enemy/Player fired a projectile!\n";
			break;
		}

		case AttackType::SwingArc:
		{
			AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(
				GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)
			);
			if (!hb) { return; }

			AEVec2 dir = { targetPos.x - pos.x, targetPos.y - pos.y };

			// If exact overlap, default to right or caster movement dir
			if (dir.x == 0.0f && dir.y == 0.0f) {
				dir = { 1.0f, 0.0f };
			} else {
				AEVec2Normalize(&dir, &dir);
			}

			AttackHitboxConfig cfg{};
			cfg.owner = caster;
			cfg.element = weapon->element;
			cfg.knockback = weapon->knockback;
			cfg.lifetime = 0.30f;
			cfg.zIndex = caster->GetZ() - 1;

			// Tentative version: circle hitbox
			cfg.colliderShape = Collision::COL_CIRCLE;
			cfg.colliderSize = { weapon->attackSize * 5.0f, weapon->attackSize * 5.0f };   // diameter (scaled up a bit for melee so it's not tiny)
			cfg.renderScale = { weapon->attackSize * 5.0f, weapon->attackSize * 5.0f };
			cfg.offset = { dir.x * (weapon->attackSize * 2.5f), dir.y * (weapon->attackSize * 2.5f) }; // offset in front
			cfg.onHit = &OnMeleeHit;
            cfg.disableOnHit = false;

			hb->Start(cfg);
            
            Bitmask bm{ caster->GetCollisionLayers() };
		    ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
		    hb->SetCollisionLayers(bm);

            std::cout << "[Combat] Enemy/Player spawned a melee hitbox!\n";
			break;
		}

        case AttackType::Stab:
		{
			AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(
				GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)
			);
			if (!hb) { return; }

			AEVec2 dir = { targetPos.x - pos.x, targetPos.y - pos.y };
			if (dir.x == 0.0f && dir.y == 0.0f) {
				dir = { 1.0f, 0.0f };
			} else {
				AEVec2Normalize(&dir, &dir);
			}

			AttackHitboxConfig cfg{};
			cfg.owner = caster;
			cfg.element = weapon->element;
			cfg.knockback = weapon->knockback;
			cfg.lifetime = 0.15f;
			cfg.zIndex = caster->GetZ() - 1;

			cfg.colliderShape = Collision::COL_RECT;
			cfg.colliderSize = { weapon->attackSize * 3.0f, weapon->attackSize * 3.0f };   
			cfg.renderScale = { weapon->attackSize * 3.0f, weapon->attackSize * 3.0f };
			cfg.offset = { dir.x * (weapon->attackSize * 4.0f), dir.y * (weapon->attackSize * 4.0f) };
			cfg.onHit = &OnMeleeHit;
            cfg.disableOnHit = false;

			hb->Start(cfg);
            
            Bitmask bm{ caster->GetCollisionLayers() };
		    ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
		    hb->SetCollisionLayers(bm);

            std::cout << "[Combat] Enemy/Player performed a Stab!\n";
			break;
		}

        case AttackType::CircleAOE:
		{
			AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(
				GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)
			);
			if (!hb) { return; }

			AttackHitboxConfig cfg{};
			cfg.owner = caster;
			cfg.element = weapon->element;
			cfg.knockback = weapon->knockback;
			cfg.lifetime = 0.30f;
			cfg.zIndex = caster->GetZ() - 1;

			cfg.colliderShape = Collision::COL_CIRCLE;
			// Massive scale to cover an area
			cfg.colliderSize = { weapon->attackSize * 8.0f, weapon->attackSize * 8.0f };   
			cfg.renderScale = { weapon->attackSize * 8.0f, weapon->attackSize * 8.0f };
			cfg.offset = { 0.0f, 0.0f }; // Centered precisely on the caster
			cfg.onHit = &OnMeleeHit;
            cfg.disableOnHit = false;
            cfg.followOwner = true; // Stay on the caster as they move

			hb->Start(cfg);
            
            Bitmask bm{ caster->GetCollisionLayers() };
		    ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
		    hb->SetCollisionLayers(bm);

            std::cout << "[Combat] Enemy/Player casted a Circle AOE!\n";
			break;
		}

		default:
			// std::cout << "Combat::ExecuteAttack: Weapon has no valid AttackType.\n";
			break;
		}
	}
}
