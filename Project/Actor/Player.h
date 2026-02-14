#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "../Inventory/Inventory.h"
#include "StatsCalc.h"
#include "../Drops/PickupGO.h"
#include "../Helpers/BitmaskUtils.h"
#include "../Inventory/EquipmentTypes.h"

class Player : public Actor
{
public:
    // This performs GameObject::Init + runtime init
    GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z,
        MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
        Bitmask _collideWithLayers, Collision::LAYER _isInLayers) override;

    void InitPlayerRuntime(const ActorStats& baseStats);

    void Update(double dt) override;
    void HandleMovementInput(double dt);
    void HandleAttackInput(double dt);
    void OnCollide(CollisionData& other) override;

    void Draw() override;

    void RecalculateStats();
    void TryPickup(const PickupPayload& payload);

    AEVec2 GetMoveDirNorm() const;

    GO_TYPE GetGOType()const override { return GO_TYPE::PLAYER; }

private:
    enum class HeldWeapon : unsigned char
    {
        Weapon1,
        Weapon2,
        Bow
    };

    const EquipmentData* GetHeldWeaponData() const;
    void DoAttackWithWeapon(const EquipmentData* weapon);

private:
    ActorStats mBaseStats{};
    Inventory  mInventory{};
    AEVec2 moveDirNorm{};

    float dodgeCDTimer{};

    // Which slot the player is currently "holding" for Left Mouse attack
    HeldWeapon heldWeapon = HeldWeapon::Weapon1;

    // Simple attack gating based on attackSpeed
    float attackCooldownTimer = 0.0f;

    float playerSpeed = 300.f;
};
