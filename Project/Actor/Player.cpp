#include "Player.h"
#include "../Helpers/Vec2Utils.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../GameDB.h"
#include "../Elements/Element.h"
#include "../Actor/Combat.h"
#include "../Drops/DropSystem.h"
#include "../DebugTools.h"
#include <iostream>

namespace {
	const float dodgeCooldown{ 0.5f };

	// Safe way to print weapon name (incase null pointer)
	static const char* SafeName(const EquipmentData* w)
	{
		return (w && w->name) ? w->name : "None";
	}
}

GameObject* Player::Init(AEVec2 _pos, AEVec2 _scale, int _z,
	MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
	Bitmask _collideWithLayers, Collision::LAYER _isInLayers)
{
	goType = GO_TYPE::PLAYER;
	return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayers);
}

void Player::InitPlayerRuntime(const ActorStats& baseStats)
{
	mBaseStats = baseStats;
	mInventory.Clear();

    const GameDB::PlayerInventoryDef& invDef = GameDB::GetPlayerStarterInventory();

    auto EquipItem = [&](int eqId, EquipmentCategory cat, auto equipFunc) {
        if (eqId > 0) {
            const EquipmentData* item = GameDB::GetEquipmentData(cat, eqId);
            if (item) {
                mInventory.AddEquipment(item);
                equipFunc(item);
				Debug::stream << "Player equipped: " << SafeName(item) << "\n";
			}
		}
	};

	EquipItem(invDef.weapon1, EquipmentCategory::Melee, [&](const EquipmentData* d) { mInventory.EquipMainWeapon(0, d); });
	EquipItem(invDef.weapon2, EquipmentCategory::Melee, [&](const EquipmentData* d) { mInventory.EquipMainWeapon(1, d); });
	EquipItem(invDef.bow, EquipmentCategory::Ranged, [&](const EquipmentData* d) { mInventory.EquipBow(d); });
	EquipItem(invDef.head, EquipmentCategory::Head, [&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.body, EquipmentCategory::Body, [&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.hands, EquipmentCategory::Hands, [&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.feet, EquipmentCategory::Feet, [&](const EquipmentData* d) { mInventory.EquipArmor(d); });

	// Give some starter ammo so bow can shoot
	mInventory.AddAmmo(50);


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
	float fdt = (float)dt;
	if (attackCooldownTimer > 0.0f)
		attackCooldownTimer -= fdt;

	HandleAttackInput(dt);
	// Track input direction for minimap arrow (Player does the actual movement)
	HandleMovementInput(dt);
	Temp_DoVelocityMovement(dt);

	Actor::Update(dt);
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

const EquipmentData* Player::GetHeldWeaponData() const
{
	switch (heldWeapon)
	{
	case HeldWeapon::Weapon1: return mInventory.GetMainWeapon(0);
	case HeldWeapon::Weapon2: return mInventory.GetMainWeapon(1);
	case HeldWeapon::Bow:     return mInventory.GetBow();
	default: return nullptr;
	}
}

void Player::HandleAttackInput(double)
{
	// Weapon selection ps: im tired of writing AEVK_*
	if (AEInputCheckTriggered('Z')) {
		heldWeapon = HeldWeapon::Weapon1;
		Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}
	if (AEInputCheckTriggered('X')) {
		heldWeapon = HeldWeapon::Weapon2;
		Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}
	if (AEInputCheckTriggered('Q')) {
		heldWeapon = HeldWeapon::Bow;
		Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}

	// Weapon swap = Right Mouse (swap weapon1 <-> weapon2)
	if (AEInputCheckTriggered(AEVK_RBUTTON)) {
		mInventory.SwapMainWeapon();

		if (heldWeapon == HeldWeapon::Weapon1) heldWeapon = HeldWeapon::Weapon2;
		else if (heldWeapon == HeldWeapon::Weapon2) heldWeapon = HeldWeapon::Weapon1;

		Debug::stream << "Swapped. Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}

	// temp location for dropping held weapon
	if (AEInputCheckTriggered('G'))
	{
		const EquipmentData* held = GetHeldWeaponData();
		if (!held) return;

		PickupPayload p{};
		p.type = DropType::Equipment;
		p.amount = 1;
		p.equipment = held;

		AEVec2 dropDir = moveDirNorm;

		// fallback if not moving
		if (dropDir.x == 0.0f && dropDir.y == 0.0f)
		{
			AEVec2 m = GetMouseWorldVec();
			dropDir = { m.x - pos.x, m.y - pos.y };

			if (dropDir.x == 0.0f && dropDir.y == 0.0f)
				dropDir = { 1.0f, 0.0f };
			else
				AEVec2Normalize(&dropDir, &dropDir);
		}
		else
		{
			AEVec2Normalize(&dropDir, &dropDir);
		}


		float dropDist = 40.0f;

		AEVec2 dropPos = GetPos();
		dropPos.x += dropDir.x * dropDist;
		dropPos.y += dropDir.y * dropDist;

		// Spawn generic item drop in the world representing what player dropped
		PickupGO::Spawn(dropPos, p);
		Debug::stream << "Dropped: " << SafeName(held) << "\n";
		Debug::stream << "After unequip held ptr=" << GetHeldWeaponData() << " name=" << SafeName(GetHeldWeaponData()) << "\n";


		// Remove the weapon completely from the active loadout slot
		switch (heldWeapon)
		{
		case HeldWeapon::Weapon1:
			mInventory.UnequipMainWeapon(0);
			break;

		case HeldWeapon::Weapon2:
			mInventory.UnequipMainWeapon(1);
			break;

		case HeldWeapon::Bow:
			mInventory.UnequipBow();
			break;
		}

		RecalculateStats();
	}

	// Attack = Left Mouse
	if (!AEInputCheckCurr(AEVK_LBUTTON)) { return; }
	if (attackCooldownTimer > 0.0f) return;
	Debug::stream << "LMB triggered. cooldown=" << attackCooldownTimer << "\n";
	if (attackCooldownTimer > 0.0f) {
		Debug::stream << "Attack pressed. Held=" << (int)heldWeapon << " weaponPtr=" << GetHeldWeaponData() << "\n";
	}
	if (!GetHeldWeaponData()) {
		Debug::stream << "No weapon equipped in held slot!\n";
		return;
	}
	if (GetHeldWeaponData()->isRanged) {
		Debug::stream << "Remaining ammo: " << mInventory.GetAmmo() - 1 << "\n";
	}

	// Ammo gate for bow/projectile weapons
	if (GetHeldWeaponData()->isRanged) {
		if (!mInventory.ConsumeAmmo(1)) {
			attackCooldownTimer = 0.0f;
			Debug::stream << "No ammo to fire!\n";
			return;
		}
	}

	// Convert attackSpeed into seconds-per-attack cooldown
	float atkSpd = mStats.attackSpeed;
	if (atkSpd <= 0.01f) atkSpd = 0.01f;
	attackCooldownTimer = 1.0f / atkSpd;

	Combat::ExecuteAttack(this, GetHeldWeaponData(), GetMouseWorldVec());
}


void Player::TryPickup(const PickupPayload& payload)
{
	switch (payload.type)
	{
	case DropType::Ammo:
		Debug::stream << "Picked up ammo " << payload.amount << '\n';
		mInventory.AddAmmo(payload.amount);
		break;

	case DropType::Heal:
		Debug::stream << "Picked up healing " << payload.amount << '\n';
		Heal((float)payload.amount);
		break;

	case DropType::Equipment:
		if (payload.equipment)
		{
			Debug::stream << "Picked up equipment " << payload.equipment->name << '\n';
			mInventory.AddEquipment(payload.equipment);

			// Auto-equip so the held slot can actually attack
			mInventory.Equip(payload.equipment);

			RecalculateStats();
		}
		break;

	case DropType::Coin:
		Debug::stream << "Picked up coin(s) " << payload.amount << '\n';
	default:
		// hook currency later
		break;
	}

	DropSystem::AddToPickupDisplay(payload);
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

void Player::Draw()
{
	Actor::Draw();
}
