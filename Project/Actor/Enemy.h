#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "EnemyDef.h"

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

private:
    // Non-owning pointer to static enemy definition data
    const EnemyDef* mDef = 0;

    // AI State tracking
    EnemyState mState = EnemyState::IDLE;
    Actor* mTarget = nullptr; // Reference to the player
    float mAttackCooldown = 0.0f; // Mock attack delay
};