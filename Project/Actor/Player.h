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

    // Recomputes the player's final stats by combining base stats, equipment modifiers, and upgrades.
    // Should be called whenever inventory changes.
    void RecalculateStats();
    
    // Processes collision with a PickupGO payload, adding items/ammo/health to the player.
    void TryPickup(const PickupPayload& payload);
    
    // Retrieves the currently wielded equipment based on the 'heldWeapon' slot state.
    const EquipmentData* GetHeldWeaponData() const;

    // Returns the normalized vector representing the direction the player is trying to move.
    AEVec2 GetMoveDirNorm() const;

    GO_TYPE GetGOType()const override { return GO_TYPE::PLAYER; }

private:
    enum class HeldWeapon : unsigned char
    {
        Weapon1,
        Weapon2,
        Bow
    };

    // Helper that executes an attack using the specified weapon.
    // Handles ammo consumption for ranged weapons and spawns appropriate hitboxes/projectiles.
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
