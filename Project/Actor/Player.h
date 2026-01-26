#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "../Inventory/Inventory.h"
#include "StatsCalc.h"
#include "../Drops/PickupGO.h"
#include "../Helpers/BitmaskUtils.h"

class Player : public Actor
{
public:
    // This performs GameObject::Init + runtime init
    GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z,
        MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
        Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers) override;

    void InitPlayerRuntime(const ActorStats& baseStats);

    void Update(double dt) override;
    void HandleMovementInput(double dt);
    void OnCollide(CollisionData& other) override;

    void RecalculateStats();
    void TryPickup(const PickupPayload& payload);

    AEVec2 GetMoveDirNorm() const;

private:
    ActorStats mBaseStats{};
    Inventory  mInventory{};
    AEVec2 moveDirNorm{};

    float dodgeCDTimer{};
};