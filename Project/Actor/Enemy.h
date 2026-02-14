#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "EnemyDef.h"

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

protected:
    // Spawns drops and disables the enemy; actual deletion is manager-controlled
    void OnDeath() override;

private:
    // Non-owning pointer to static enemy definition data
    const EnemyDef* mDef = 0;
};