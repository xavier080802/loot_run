#pragma once
#include "AEEngine.h"
#include "Actor.h"
#include "../Inventory/Inventory.h"
#include "StatsCalc.h"
#include "../Drops/PickupGO.h"
#include "../Helpers/BitmaskUtils.h"
#include "../Inventory/EquipmentTypes.h"
#include "../InputManager.h"

class Player : public Actor, Input::InputSub
{
public:
    // This performs GameObject::Init + runtime init
    GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z,
        MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
        Bitmask _collideWithLayers, Collision::LAYER _isInLayers) override;

    void InitPlayerRuntime(const ActorStats& baseStats);

    void Update(double dt) override;
    void HandleMovementInput(double dt);
    void OnCollide(CollisionData& other) override;

    void Draw() override;

    bool IsInvulnerable() override;

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
    const ActorStats& GetBaseStats() const override { return mBaseStats; }

    void SubscriptionAlert(Input::InputKeyData content) override;

    ~Player() {};

private:
    enum class HeldWeapon : unsigned char
    {
        Weapon1,
        Weapon2,
        Bow
    };

private:
    ActorStats mBaseStats{};
    Inventory  mInventory{};
    AEVec2 moveDirNorm{};

    float dodgeCDTimer{};
    float dodgeIFrameTimer{};

    // Which slot the player is currently "holding" for Left Mouse attack
    HeldWeapon heldWeapon = HeldWeapon::Weapon1;

    // Simple attack gating based on attackSpeed
    float attackCooldownTimer = 0.0f;

    float playerSpeed = 300.f;
};
