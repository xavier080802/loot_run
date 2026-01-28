#include "Enemy.h"
#include "../Drops/DropSystem.h"

GameObject* Enemy::Init(AEVec2 _pos, AEVec2 _scale, int _z,
    MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
    Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers)
{
    goType = GO_TYPE::ENEMY;
    return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize,
        _collideWithLayers, _isInLayers);
}

void Enemy::InitEnemyRuntime(const EnemyDef* def)
{
    mDef = def;
    if (!mDef) return;

    InitActorRuntime(mDef->baseStats);
}

void Enemy::Update(double dt)
{
    GameObject::Update(dt);
    // TODO: AI later (chase player, etc.)
}

void Enemy::OnDeath()
{
    if (mDef)
        DropSystem::SpawnDrops(mDef->dropTableId, GetPos());

    // Disable (manager keeps ownership / avoids delete mid-frame)
    isEnabled = false;
    collisionEnabled = false;
}