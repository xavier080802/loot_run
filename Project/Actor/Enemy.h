#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "EnemyDef.h"

class Enemy : public Actor
{
public:
    GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z,
        MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
        Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers) override;

    void InitEnemyRuntime(const EnemyDef* def);

    virtual void Update(double dt) override;
    virtual void Free() override;

protected:
    void OnDeath() override;

private:
    const EnemyDef* mDef = 0;
};