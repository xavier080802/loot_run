#include "Player.h"
#include "../Helpers/Vec2Utils.h"
#include "../GameObjects/Projectile.h"

namespace {
    const float dodgeCooldown{ 0.5f };
}

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
}

void Player::HandleMovementInput(double dt)
{
    float fdt = (float)dt;

    if (dodgeCDTimer > 0.f) {
        dodgeCDTimer -= fdt;
    }

    AEVec2 dir{ 0.0f, 0.0f };
    if (AEInputCheckCurr('W')) dir.y += 1.0f;
    if (AEInputCheckCurr('S')) dir.y -= 1.0f;
    if (AEInputCheckCurr('A')) dir.x -= 1.0f;
    if (AEInputCheckCurr('D')) dir.x += 1.0f;
    if (dodgeCDTimer <= 0.f && AEInputCheckTriggered(AEVK_SPACE)) {
        ApplyForce(dir * 500.f);
        dodgeCDTimer = dodgeCooldown;
    }

    if (dir.x || dir.y) {
        AEVec2Normalize(&dir, &dir);
    }
    moveDirNorm = dir;

    AEVec2 p = GetPos();
    p.x += dir.x * mStats.moveSpeed * fdt;
    p.y += dir.y * mStats.moveSpeed * fdt;
    SetPos(p);
}

static void Test_ProjHitCallback(GameObject::CollisionData& data) {
    if (data.other.GetGOType() != GO_TYPE::ENEMY) return;
    //Do dmg.
    //TODO: get damage calc from weapon and buffs etc.
    Actor& target = dynamic_cast<Actor&>(data.other);
    target.TakeDamage(10);
}

void Player::HandleAttackInput(double dt)
{
    //Testing projectiles.
    //The collide reaction should be controlled by the weapon through OnHitCallback
    if (AEInputCheckTriggered(AEVK_F)) {
        Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
        AEVec2 m = GetMouseVec();
        //Fire at cursor
        proj->Fire(this, { m.x - pos.x, m.y - pos.y }, 10, 200, 3, Test_ProjHitCallback);
    }
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

AEVec2 Player::GetMoveDirNorm() const
{
    return moveDirNorm;
}

void Player::OnCollide(CollisionData& other)
{
    switch (other.other.GetGOType())
    {
    case GO_TYPE::ENEMY:
        // contact damage, knockback, etc.
        break;
    case GO_TYPE::ITEM: {
        PickupGO* pickup = dynamic_cast<PickupGO*>(&other.other);
        if (pickup)
        {
            TryPickup(pickup->GetPayload());
            // PickupGO::OnCollide will disable itself when it receives the collision event.
        }

        break;
    }
    case GO_TYPE::LOOT_CHEST: //LootChest handles this.
    default:
        break;
    }
}