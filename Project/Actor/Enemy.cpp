#include "Enemy.h"
#include "../Drops/DropSystem.h"

GameObject* Enemy::Init(AEVec2 _pos, AEVec2 _scale, int _z,
    MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
    Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers)
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
    GameObject::Update(dt);
    // TODO: AI later (chase player, etc.)
}

void Enemy::OnDeath()
{
    // Drop logic is data-driven via EnemyDef
    if (mDef)
        DropSystem::SpawnDrops(mDef->dropTableId, GetPos());

    // Disable (manager keeps ownership / avoids delete mid-frame)
    Actor::OnDeath();
}

void Enemy::Free()
{
    mDef = nullptr;
    Actor::Free();
}
