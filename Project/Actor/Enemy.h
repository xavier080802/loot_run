#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "EnemyDef.h"
#include "../GameDB.h"

// Basic AI states for the enemy
enum class EnemyState {
    IDLE,
    CHASE,
    ATTACK
};

// Runtime enemy entity driven by an immutable EnemyDef
class Enemy : public Actor
{
public:
    GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z,
        MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
        Bitmask _collideWithLayers, Collision::LAYER _isInLayers) override;

    // Binds this enemy instance to its data definition and initializes stats
    void InitEnemyRuntime(const EnemyDef* def);

    virtual void Update(double dt) override;
    virtual void Free() override;

    EnemyDef const& GetDefinition() const { return *mDef; }
    const ActorStats& GetBaseStats() const override { return mDef->baseStats; }

    virtual ~Enemy() {};

protected:
    // Spawns drops and disables the enemy; actual deletion is manager-controlled
    void OnDeath(Actor* killer = nullptr) override;

    void OnStatEffectChange() override;
    ActorStats CalculateStatusEffectStats();

private:
    // Non-owning pointer to static enemy definition data
    const EnemyDef* mDef = 0;

    // AI State tracking
    EnemyState mState = EnemyState::IDLE;
    Actor* mTarget = nullptr; // Reference to the player
    float mAttackCooldown = 0.0f; // Mock attack delay
};

// Enemy spawn helpers

/// Creates an Enemy from an explicit EnemyDef pointer.
/// Returns nullptr if def is null.
Enemy* SpawnEnemyFromDef(const EnemyDef* def, AEVec2 pos);

/// Picks a uniformly random Normal-tier enemy and spawns it.
Enemy* SpawnRandomNormalEnemy(AEVec2 pos);

/// Picks a uniformly random Elite-tier enemy and spawns it.
Enemy* SpawnRandomEliteEnemy(AEVec2 pos);

/// Picks a uniformly random Boss-tier enemy and spawns it.
Enemy* SpawnRandomBossEnemy(AEVec2 pos);

/// Weighted roll: normalChance for Normal, eliteChance for Elite, no Boss.
/// Returns nullptr if the roll falls outside both bands.
Enemy* SpawnWeightedEnemy(AEVec2 pos, float normalChance = 0.70f, float eliteChance = 0.30f);