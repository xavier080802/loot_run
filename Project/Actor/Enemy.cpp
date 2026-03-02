#include "Enemy.h"
#include "../Drops/DropSystem.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Actor/Combat.h"

GameObject* Enemy::Init(AEVec2 _pos, AEVec2 _scale, int _z,
    MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
    Bitmask _collideWithLayers, Collision::LAYER _isInLayers)
{
    // Used by collision, filtering, and game logic to identify enemy objects
    goType = GO_TYPE::ENEMY;
    return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize,
        _collideWithLayers, _isInLayers);
}

void Enemy::InitEnemyRuntime(const EnemyDef* def)
{
    // EnemyDef is expected to outlive this Enemy instance
    mDef = def;
    if (!mDef) return;

    InitActorRuntime(mDef->baseStats);
}

void Enemy::Update(double dt)
{
    // Handles knockback or impulse-based movement
    Temp_DoVelocityMovement(dt);
    Actor::Update(dt);

    // Basic AI State Machine: Find the player if we don't have one
    if (!mTarget || mTarget->IsDead() || !mTarget->IsEnabled()) {
        mTarget = nullptr;
        // Search for player
        for (GameObject* go : GameObjectManager::GetInstance()->GetGameObjects()) {
            if (go->GetGOType() == GO_TYPE::PLAYER && go->IsEnabled()) {
                mTarget = dynamic_cast<Actor*>(go);
                break;
            }
        }
    }

    if (mTarget) {
        AEVec2 tPos = mTarget->GetPos();
        AEVec2 myPos = GetPos();
        AEVec2 dir = { tPos.x - myPos.x, tPos.y - myPos.y };
        float distSq = dir.x * dir.x + dir.y * dir.y;
        float attackRange = mDef ? mDef->attack.range : 50.0f; // Range at which enemy stops to attack

        // Prevent enemy from walking into the player if the range is too small
        float minNonOverlapDist = (GetColSize().x * 0.5f) + (mTarget->GetColSize().x * 0.5f);
        if (attackRange < minNonOverlapDist) {
            if (mDef && mDef->attack.mockWeapon.attackType == AttackType::SwingArc) {
                // Auto calculate optimal stopping distance based on melee reach
                attackRange = mDef->attack.mockWeapon.attackSize * 5.0f;
            } else if (mDef && mDef->attack.mockWeapon.attackType == AttackType::Projectile) {
                // Auto calculate optimal shooting distance
                attackRange = mDef->attack.mockWeapon.attackSize * 15.0f;
            } else if (mDef && mDef->attack.mockWeapon.attackType == AttackType::Stab) {
                // Stab offset is 4.0x size, with a 1.5x physical radius.
                attackRange = (mDef->attack.mockWeapon.attackSize * 4.0f) + (mDef->attack.mockWeapon.attackSize * 1.5f);
            } else if (mDef && mDef->attack.mockWeapon.attackType == AttackType::CircleAOE) {
                // Circle AOE is centered, simply based on its radius (half of its 8.0 size multiplier).
                attackRange = mDef->attack.mockWeapon.attackSize * 4.0f;
            }
            
            // Final check to ensure they never overlap
            if (attackRange < minNonOverlapDist) {
                attackRange = minNonOverlapDist;
            }
        }

        if (mState == EnemyState::IDLE) {
            // Constant aggro for now (can add checks based on distance later)
            mState = EnemyState::CHASE;
        } 
        else if (mState == EnemyState::CHASE) {
            // Move towards player
            if (distSq <= attackRange * attackRange) {
                mState = EnemyState::ATTACK;
                if (mDef) {
                    mAttackCooldown = mDef->attack.cooldown;
                    Combat::ExecuteAttack(this, &mDef->attack.mockWeapon, mTarget->GetPos());
                }
            } else {
                if (dir.x != 0 || dir.y != 0) {
                    AEVec2Normalize(&dir, &dir);
                    myPos.x += dir.x * mStats.moveSpeed * (float)dt;
                    myPos.y += dir.y * mStats.moveSpeed * (float)dt;
                    SetPos(myPos);
                }
            }
        } 
        else if (mState == EnemyState::ATTACK) {
            // "Attack pattern" delay mock
            mAttackCooldown -= (float)dt;
            if (mAttackCooldown <= 0.0f) {
                // Re-evaluate state after attack is done
                mState = EnemyState::CHASE;
            }
        }
    } else {
        mState = EnemyState::IDLE;
    }
}

void Enemy::OnDeath(Actor* killer)
{
    // Drop logic is data-driven via EnemyDef
    if (mDef)
        DropSystem::SpawnDrops(mDef->dropTableId, GetPos());

    // Call base class OnDeath to trigger subscriptions
    Actor::OnDeath(killer);
}

void Enemy::Free()
{
    mDef = nullptr;
    Actor::Free();
}
