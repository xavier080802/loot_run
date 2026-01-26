#include "Player.h"

GameObject* Player::Init(AEVec2 _pos, AEVec2 _scale, int _z,
    MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
    Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers)
{
    return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayers);
}

void Player::InitPlayerRuntime(const ActorStats& baseStats)
{
    mBaseStats = baseStats;
    mInventory.Clear();

    RecalculateStats();
    InitActorRuntime(mStats);
}

void Player::RecalculateStats()
{
    EquipmentModifiers eq = mInventory.GetEquipmentModifiers();
    UpgradeMultipliers up = mInventory.GetUpgradeMultipliers();

    mStats = StatsCalc::ComputeFinalStats(mBaseStats, eq, up);

    // Keep HP valid after stat changes
    if (mCurrentHP > mStats.maxHP) mCurrentHP = mStats.maxHP;
    if (mCurrentHP <= 0.0f) mCurrentHP = mStats.maxHP;
}

void Player::Update(double dt)
{
    GameObject::Update(dt);

    float fdt = (float)dt;

    AEVec2 dir{ 0.0f, 0.0f };
    if (AEInputCheckCurr('W')) dir.y += 1.0f;
    if (AEInputCheckCurr('S')) dir.y -= 1.0f;
    if (AEInputCheckCurr('A')) dir.x -= 1.0f;
    if (AEInputCheckCurr('D')) dir.x += 1.0f;

    float lenSq = dir.x * dir.x + dir.y * dir.y;
    if (lenSq > 1.0f)
    {
        float invLen = 1.0f / sqrtf(lenSq);
        dir.x *= invLen;
        dir.y *= invLen;
    }

    AEVec2 p = GetPos();
    p.x += dir.x * mStats.moveSpeed * fdt;
    p.y += dir.y * mStats.moveSpeed * fdt;
    SetPos(p);
}

void Player::TryPickup(const PickupPayload& payload)
{
    switch (payload.type)
    {
    case DropType::Ammo:
        mInventory.AddAmmo(payload.amount);
        break;

    case DropType::Heal:
        Heal((float)payload.amount);
        break;

    case DropType::Equipment:
        if (payload.equipment)
        {
            mInventory.AddEquipment(payload.equipment);
            RecalculateStats();
        }
        break;

    case DropType::Coin:
    default:
        // hook currency later
        break;
    }
}

void Player::OnCollide(CollisionData& other)
{
    // Pickup collision: currently using COLLISION_LAYER::PET as "pickup layer".
    if (other.other.GetColliderLayer() == GameObject::COLLISION_LAYER::PET)
    {
        PickupGO* pickup = dynamic_cast<PickupGO*>(&other.other);
        if (pickup)
        {
            TryPickup(pickup->GetPayload());
            // PickupGO::OnCollide will disable itself when it receives the collision event.
        }
        return;
    }

    if (other.other.GetColliderLayer() == GameObject::COLLISION_LAYER::ENEMIES)
    {
        // contact damage, knockback, etc.
    }
}