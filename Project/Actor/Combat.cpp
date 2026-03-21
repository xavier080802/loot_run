#include "Combat.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Elements/Element.h"
#include <iostream>

namespace Combat
{
	/**
 * @brief Called automatically when a projectile hits something.
 *
 * When a player fires an arrow and it collides with an enemy (or vice versa),
 * the Projectile object's collision callback fires this function.
 * It filters friendly fire (player can't shoot player, enemy can't shoot enemy),
 * then uses Actor::DealDamage to apply physical damage.
 * If the weapon had an element, it also applies that status effect on top.
 *
 * @param data     Info about what the projectile hit (position, ref to the other object).
 *                 Passed by REFERENCE so we can read the actual target object without copying.
 * @param caster   The actor who fired the projectile. Passed as a raw POINTER because
 *                 the projectile needs to know who's responsible for the damage.
 * @param element  What elemental effect to apply on hit (or NONE if no element).
 *                 Passed by VALUE.
 * @param knockback Unused parameter, included due to function signature.
 *
 * @note Called by:
 *   - Projectile::OnCollide() (in Projectile.cpp). The projectile object fires this
 *     when its physics volume touches another object.
 */
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

	/**
	 * @brief Called automatically when a melee hitbox touches something.
	 *
	 * This is the melee equivalent of OnProjectileHit. When AttackHitboxGO collides
	 * with a valid target, it fires this. After dealing damage, it also calculates
	 * and applies knockback, pushing the target away from the attacker.
	 * Bigger enemies (high max HP) receive less knockback to avoid them being sent flying.
	 *
	 * @param data     Info about what was hit. Passed by REFERENCE to avoid copying
	 *                 and to allow direct access to the hit object.
	 * @param caster   The actor who swung the weapon. Passed as POINTER (no ownership).
	 * @param element  Elemental effect to apply. Passed by VALUE (small enum).
	 * @param knockback How hard to push the target. Passed by VALUE (float).
	 * @param extra    Optional extra data for future use (unused right now).
	 *
	 * @note Called by:
	 *   - AttackHitboxGO::OnCollide() - the hitbox fires this as its onHit callback.
	 *     Set up during Combat::ExecuteAttack() when creating the hitbox config.
	 */
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

	/**
	 * @brief Routes an attack to the right spawner depending on the weapon type.
	 *
	 * This is the main entry point for any character wanting to attack.
	 * Given a weapon and a world-space aim position, it:
	 * - Fetches a pooled game object (Projectile or AttackHitboxGO)
	 * - Configures it based on the weapon's attack type
	 * - Launches it toward the target
	 *
	 * Attack types handled:
	 *   - Projectile: Fires a flying bullet/arrow toward targetPos.
	 *   - SwingArc: Creates a circular hitbox in front of the caster.
	 *   - Stab: Creates a rectangular hitbox directly in front of the caster.
	 *   - CircleAOE: Creates a large circular hitbox centered on the caster.
	 *
	 * @param caster    The character doing the attacking. Passed as a raw POINTER
	 *                  so we can query their position and collision layers.
	 * @param casterPos Position where the attack will be generated from.
	 * @param weapon    The item being used to attack. Passed as a CONST POINTER.
	 *                  Read its properties (size, type, element) but never change it.
	 * @param targetPos The world position being aimed at (e.g. mouse cursor for player,
	 *                  player's position for enemy). Passed by VALUE.
	 *
	 * @note Called by:
	 *   - Player::Update() - when the player left-clicks to attack.
	 *   - Enemy::Update() - when the enemy's attack cooldown expires and it's in attack range.
	 */
	void ExecuteAttack(Actor* caster, AEVec2 casterPos, const EquipmentData* weapon, AEVec2 targetPos) {
		if (!caster || !weapon) return;

		AEVec2 pos = casterPos;

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
			}
			else {
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
			cfg.offset = (pos - caster->GetPos()) + AEVec2{ dir.x* (weapon->attackSize * 2.5f), dir.y* (weapon->attackSize * 2.5f) }; // offset in front
			cfg.onHit = &OnMeleeHit;
			cfg.disableOnHit = false;

			hb->Start(cfg);

			Bitmask bm{ caster->GetCollisionLayers() };
			ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
			ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE);
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
			}
			else {
				AEVec2Normalize(&dir, &dir);
			}

			AttackHitboxConfig cfg{};
			cfg.owner = caster;
			cfg.element = weapon->element;
			cfg.knockback = weapon->knockback;
			cfg.lifetime = 0.15f;
			cfg.zIndex = caster->GetZ() - 1;

			cfg.colliderShape = Collision::COL_CIRCLE;
			cfg.colliderSize = { weapon->attackSize * 3.0f, weapon->attackSize * 3.0f };
			cfg.meshShape = MESH_SQUARE;
			cfg.renderScale = { weapon->attackSize * 6.0f, weapon->attackSize * 1.5f };
			cfg.offset = { dir.x * (weapon->attackSize * 4.0f), dir.y * (weapon->attackSize * 4.0f) };
			cfg.onHit = &OnMeleeHit;
			cfg.disableOnHit = false;

			hb->Start(cfg);
			hb->SetRotation(AERadToDeg(atan2(dir.y, dir.x)));

			Bitmask bm{ caster->GetCollisionLayers() };
			ResetFlagAtPos(&bm, Collision::LAYER::INTERACTABLE);
			ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE);
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
			ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE);
			hb->SetCollisionLayers(bm);

			std::cout << "[Combat] Enemy/Player casted a Circle AOE!\n";
			break;
		}

		default:
			// std::cout << "Combat::ExecuteAttack: Weapon has no valid AttackType.\n";
			break;
		}
	}

	/**
	 * @brief Routes an attack to the right spawner depending on the weapon type.
	 *
	 * This is the main entry point for any character wanting to attack.
	 * Given a weapon and a world-space aim position, it:
	 * - Fetches a pooled game object (Projectile or AttackHitboxGO)
	 * - Configures it based on the weapon's attack type
	 * - Launches it toward the target
	 *
	 * Attack types handled:
	 *   - Projectile: Fires a flying bullet/arrow toward targetPos.
	 *   - SwingArc: Creates a circular hitbox in front of the caster.
	 *   - Stab: Creates a rectangular hitbox directly in front of the caster.
	 *   - CircleAOE: Creates a large circular hitbox centered on the caster.
	 *
	 * @param caster    The character doing the attacking. Passed as a raw POINTER
	 *                  so we can query their position and collision layers.
	 * @param weapon    The item being used to attack. Passed as a CONST POINTER.
	 *                  Read its properties (size, type, element) but never change it.
	 * @param targetPos The world position being aimed at (e.g. mouse cursor for player,
	 *                  player's position for enemy). Passed by VALUE.
	 *
	 * @note Called by:
	 *   - Player::Update() - when the player left-clicks to attack.
	 *   - Enemy::Update() - when the enemy's attack cooldown expires and it's in attack range.
	 */
	void ExecuteAttack(Actor* caster, const EquipmentData* weapon, AEVec2 targetPos)
	{
		ExecuteAttack(caster, caster->GetPos(), weapon, targetPos);
	}
}
