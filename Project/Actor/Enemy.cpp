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
    return Actor::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize,
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
            if (mDef && mDef->attack.mockWeapon.attackType == ATTACK_TYPE::SWINGARC) {
                // Auto calculate optimal stopping distance based on melee reach
                attackRange = mDef->attack.mockWeapon.attackSize * 5.0f;
            } else if (mDef && mDef->attack.mockWeapon.attackType == ATTACK_TYPE::PROJECTILE) {
                // Auto calculate optimal shooting distance
                attackRange = mDef->attack.mockWeapon.attackSize * 15.0f;
            } else if (mDef && mDef->attack.mockWeapon.attackType == ATTACK_TYPE::STAB) {
                // Stab offset is 4.0x size, with a 1.5x physical radius.
                attackRange = (mDef->attack.mockWeapon.attackSize * 4.0f) + (mDef->attack.mockWeapon.attackSize * 1.5f);
            } else if (mDef && mDef->attack.mockWeapon.attackType == ATTACK_TYPE::CIRCLE_AOE) {
                // Circle AOE is centered, simply based on its radius (half of its 8.0 size multiplier).
                attackRange = mDef->attack.mockWeapon.attackSize * 4.0f;
            }
            
            // Final check to ensure they never overlap
            if (attackRange < minNonOverlapDist) {
                attackRange = minNonOverlapDist;
            }
        }

        if (mState == ENEMY_STATE::IDLE) {
            // Wake up and chase if player enters aggro range
            float aggroRange = mDef ? mDef->attack.aggroRange : 500.0f;
            if (distSq <= aggroRange * aggroRange) {
                mState = ENEMY_STATE::CHASE;
            }
        } 
        else if (mState == ENEMY_STATE::CHASE) {
            // Move towards player
            if (distSq <= attackRange * attackRange) {
                mState = ENEMY_STATE::ATTACK;
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
        else if (mState == ENEMY_STATE::ATTACK) {
            // "Attack pattern" delay mock
            mAttackCooldown -= (float)dt;
            if (mAttackCooldown <= 0.0f) {
                // Re-evaluate state after attack is done
                mState = ENEMY_STATE::CHASE;
            }
        }
    } else {
        mState = ENEMY_STATE::IDLE;
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

void Enemy::OnStatEffectChange()
{
    if (!mDef) return;
    mStats = mDef->baseStats;
    mStats.maxHP *= mDifficultyMultiplier;
    mStats.attack *= mDifficultyMultiplier;
    mStats.defense *= mDifficultyMultiplier;
    mStats.moveSpeed *= mDifficultyMultiplier;
    //Recalc stat changes
    ActorStats seStats = CalculateStatusEffectStats();
    mStats.attack += seStats.attack;
    mStats.attackSpeed += seStats.attackSpeed;
    mStats.defense += seStats.defense;
    mStats.maxHP += seStats.maxHP;
    mStats.moveSpeed += seStats.moveSpeed;
    ClampStats();
}

ActorStats Enemy::CalculateStatusEffectStats()
{
    ActorStats out;
    for (auto const& p : statusEffectsDict) {
        out.attack += p.second->GetFinalModVal(STAT_TYPE::ATT, mDef->baseStats.attack);
        out.attackSpeed += p.second->GetFinalModVal(STAT_TYPE::ATT_SPD, mDef->baseStats.attackSpeed);
        out.defense += p.second->GetFinalModVal(STAT_TYPE::DEF, mDef->baseStats.defense);
        out.maxHP += p.second->GetFinalModVal(STAT_TYPE::MAX_HP, mDef->baseStats.maxHP);
        out.moveSpeed += p.second->GetFinalModVal(STAT_TYPE::MOVE_SPD, mDef->baseStats.moveSpeed);
    }
    return out;
}

void Enemy::Free()
{
    mDef = nullptr;
    Actor::Free();
}

/**
 * @brief Core spawn function. Creates, initialises, and returns a live Enemy.
 *
 * Mirrors the SpawnEnemyFromDef lambda previously duplicated in GameState.cpp.
 * Reads mesh/collision shape from the def's render data and calls Init() +
 * InitEnemyRuntime() on the new Enemy.
 *
 * @param def  Immutable definition for the enemy type to spawn. CONST POINTER.
 * @param pos  World-space spawn position. Passed by VALUE.
 *
 * @return Pointer to the newly allocated Enemy, or nullptr if def is null.
 *
 * @note Called by:
 *   - SpawnRandomNormalEnemy / SpawnRandomEliteEnemy / SpawnRandomBossEnemy / SpawnWeightedEnemy
 *   - GameState::InitState() (replaces the old local lambda)
 */
Enemy* SpawnEnemyFromDef(const EnemyDef* def, AEVec2 pos)
{
    if (!def) return nullptr;

    float r = def->render.radius;
    MESH_SHAPE  meshShape = (def->render.mesh == ENEMY_MESH::SQUARE) ? MESH_SQUARE : MESH_CIRCLE;
    Collision::SHAPE colShape = (meshShape == MESH_SQUARE) ? Collision::COL_RECT : Collision::COL_CIRCLE;

    Enemy* enemy = new Enemy();
    enemy->Init(
        pos,
        { r * 2.0f, r * 2.0f },
        0,
        meshShape,
        colShape,
        { r * 2.0f, r * 2.0f },
        CreateBitmask(2, Collision::PLAYER, Collision::OBSTACLE),
        Collision::ENEMIES
    );
    enemy->InitEnemyRuntime(def);

    // Set texture after InitEnemyRuntime so it is not overwritten
    enemy->GetRenderData().AddTexture(def->render.texturePath.empty()
        ? "Assets/enemyplaceholder.png"
        : def->render.texturePath.c_str());
    enemy->GetRenderData().SetActiveTexture(0);
    enemy->GetRenderData().tint = CreateColor(255, 255, 255, 255);
    enemy->GetRenderData().alpha = 255;

    return enemy;
}

/**
 * @brief Spawns a uniformly random Normal-tier enemy at the given position.
 *
 * @param pos  World-space spawn position. Passed by VALUE.
 * @return Newly allocated Enemy*, or nullptr if no Normal enemies are registered.
 *
 * @note Called by:
 *   - Wave spawners / GameState when a Normal encounter is needed.
 */
Enemy* SpawnRandomNormalEnemy(AEVec2 pos)
{
    return SpawnEnemyFromDef(GameDB::GetRandomNormalEnemy(), pos);
}

/**
 * @brief Spawns a uniformly random Elite-tier enemy at the given position.
 *
 * @param pos  World-space spawn position. Passed by VALUE.
 * @return Newly allocated Enemy*, or nullptr if no Elite enemies are registered.
 *
 * @note Called by:
 *   - Wave spawners / GameState when an Elite encounter is needed.
 */
Enemy* SpawnRandomEliteEnemy(AEVec2 pos)
{
    return SpawnEnemyFromDef(GameDB::GetRandomEliteEnemy(), pos);
}

/**
 * @brief Spawns a uniformly random Boss-tier enemy at the given position.
 *
 * @param pos  World-space spawn position. Passed by VALUE.
 * @return Newly allocated Enemy*, or nullptr if no Boss enemies are registered.
 *
 * @note Called by:
 *   - Boss-room / end-of-level spawner.
 */
Enemy* SpawnRandomBossEnemy(AEVec2 pos)
{
    return SpawnEnemyFromDef(GameDB::GetRandomBossEnemy(), pos);
}

/**
 * @brief Spawns a Normal or Elite enemy using weighted probability.
 *
 * Delegates the tier roll to GameDB::GetWeightedRandomEnemy(). Boss enemies
 * are intentionally excluded and must be spawned via SpawnRandomBossEnemy().
 *
 * @param pos          World-space spawn position. Passed by VALUE.
 * @param normalChance Probability [0..1] of selecting Normal (default 70%).
 * @param eliteChance  Probability [0..1] of selecting Elite  (default 30%).
 *
 * @return Newly allocated Enemy*, or nullptr if the roll misses both bands.
 *
 * @note Called by:
 *   - Generic wave spawners that want tier-weighted variety without Bosses.
 */
Enemy* SpawnWeightedEnemy(AEVec2 pos, float normalChance, float eliteChance)
{
    return SpawnEnemyFromDef(GameDB::GetWeightedRandomEnemy(normalChance, eliteChance), pos);
}
