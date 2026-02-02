#include "Player.h"
#include "../Helpers/Vec2Utils.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../GameDB.h"
#include <iostream>

namespace {
	const float dodgeCooldown{ 0.5f };

	// Fire() requires a plain function pointer, so this must NOT be a capturing lambda.
	void OnProjectileHit(GameObject::CollisionData& data, Actor* caster)
	{
		if (!caster) return;
		if (data.other.GetGOType() != GO_TYPE::ENEMY) return;

		Actor& target = static_cast<Actor&>(data.other);
		target.TakeDamage(caster->GetStats().attack);
	}

	void OnMeleeHit(GameObject::CollisionData& data, Actor* caster)
	{
		if (!caster) return;
		if (data.other.GetGOType() != GO_TYPE::ENEMY) return;

		Actor& target = static_cast<Actor&>(data.other);
		target.TakeDamage(caster->GetStats().attack);

		//Knockback
		AEVec2 dir = {
		target.GetPos().x - caster->GetPos().x,
		target.GetPos().y - caster->GetPos().y
		};

		if (dir.x != 0.0f || dir.y != 0.0f)
			AEVec2Normalize(&dir, &dir);

		float knockbackForce = 100.0f;
		if (target.GetMaxHP() > 200.0f) {
			knockbackForce *= 0.4f;
		}

		target.ApplyForce({ dir.x * knockbackForce, dir.y * knockbackForce });
	}

	// Safe way to print weapon name (incase null pointer)
	static const char* SafeName(const EquipmentData* w)
	{
		return (w && w->name) ? w->name : "None";
	}
}

GameObject* Player::Init(AEVec2 _pos, AEVec2 _scale, int _z,
	MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize,
	Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers)
{
	goType = GO_TYPE::PLAYER;
	return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayers);
}

void Player::InitPlayerRuntime(const ActorStats& baseStats)
{
	mBaseStats = baseStats;
	mInventory.Clear();

	// Give starter loadout
	const EquipmentData* sword = GameDB::GetEquipmentData(1);
	const EquipmentData* bow = GameDB::GetEquipmentData(3);

	if (sword) {
		mInventory.AddEquipment(sword);
		mInventory.EquipMainWeapon(0, sword);// Weapon1 slot
	}

	if (bow) {
		mInventory.AddEquipment(bow);
		mInventory.EquipBow(bow);// Bow slot
		//checking if bow equip worked
		std::cout << "bow ptr=" << bow << " name=" << SafeName(bow) << "\n";
	}

	// Give some starter ammo so bow can shoot
	mInventory.AddAmmo(50);

	RecalculateStats();
	InitActorRuntime(mStats);
}


void Player::RecalculateStats()
{
	EquipmentModifiers eq = mInventory.GetEquipmentModifiers();
	UpgradeMultipliers up = mInventory.GetUpgradeMultipliers();
	//TODO: status effect calculations
	mStats = StatsCalc::ComputeFinalStats(mBaseStats, eq, up);

	// Keep HP valid after stat changes
	if (mCurrentHP > mStats.maxHP) mCurrentHP = mStats.maxHP;
	if (mCurrentHP <= 0.0f) mCurrentHP = mStats.maxHP;
}

void Player::Update(double dt)
{
	Actor::Update(dt);

	float fdt = (float)dt;
	if (attackCooldownTimer > 0.0f)
		attackCooldownTimer -= fdt;
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

void Player::HandleAttackInput(double dt)
{
	(void)dt;

	// Weapon selection ps: im tired of writing AEVK_*
	if (AEInputCheckTriggered('Z')) {
		heldWeapon = HeldWeapon::Weapon1;
		std::cout << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}
	if (AEInputCheckTriggered('X')) {
		heldWeapon = HeldWeapon::Weapon2;
		std::cout << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}
	if (AEInputCheckTriggered('Q')) {
		heldWeapon = HeldWeapon::Bow;
		std::cout << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
	}

	// Weapon swap = Right Mouse (swap weapon1 <-> weapon2)
	if (AEInputCheckTriggered(AEVK_RBUTTON)) {
		mInventory.SwapMainWeapon();

		if (heldWeapon == HeldWeapon::Weapon1) heldWeapon = HeldWeapon::Weapon2;
		else if (heldWeapon == HeldWeapon::Weapon2) heldWeapon = HeldWeapon::Weapon1;

		std::cout << "Swapped. Held: " << SafeName(GetHeldWeaponData()) << "\n";
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

		
		PickupGO::Spawn(dropPos, p);
		std::cout << "Dropped: " << SafeName(held) << "\n";
		std::cout << "After unequip held ptr=" << GetHeldWeaponData() << " name=" << SafeName(GetHeldWeaponData()) << "\n";


		//Unequip
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
	if (!AEInputCheckTriggered(AEVK_LBUTTON)) { return; }
	if (attackCooldownTimer > 0.0f) return;
	std::cout << "LMB triggered. cooldown=" << attackCooldownTimer << "\n";
	if (attackCooldownTimer > 0.0f) {
		std::cout << "Attack pressed. Held=" << (int)heldWeapon << " weaponPtr=" << GetHeldWeaponData() << "\n";
	}
	if (!GetHeldWeaponData()) {
		std::cout << "No weapon equipped in held slot!\n";
		return;
	}
	if (GetHeldWeaponData()->isRanged) {
		std::cout << "Remaining ammo: " << mInventory.GetAmmo() - 1 << "\n";
	}

	DoAttackWithWeapon(GetHeldWeaponData());
}

void Player::DoAttackWithWeapon(const EquipmentData* weapon)
{
	if (!weapon) return;

	//Check if subscribers want to cancel the cast.
	bool allowCast{ true };
	for (ActorBeforeCastSub* sub : beforeCastSubs) {
		sub->SubscriptionAlert({ allowCast, this, weapon });
		if (!allowCast) return;
	}

	// Convert attackSpeed into seconds-per-attack cooldown
	float atkSpd = mStats.attackSpeed;
	if (atkSpd <= 0.01f) atkSpd = 0.01f;
	attackCooldownTimer = 1.0f / atkSpd;

	switch (weapon->attackType)
	{
	case AttackType::Projectile:
	{
		// Ammo gate for bow/projectile weapons
		if (!mInventory.ConsumeAmmo(1)) {
			attackCooldownTimer = 0.0f;
			return;
		}

		Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
		if (!proj) return;

		AEVec2 m = GetMouseWorldVec();
		AEVec2 fireDir = { m.x - pos.x, m.y - pos.y };

		// Fire(caster, direction, radius, speed, lifetime, callback)
		proj->Fire(this, fireDir, 10.0f, 200.0f, 3.0f, &OnProjectileHit);
		break;
	}

	case AttackType::SwingArc:
	{
		AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(
			GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)
			);
		if (!hb) { return; }

		AEVec2 m = GetMouseWorldVec();
		AEVec2 dir = { m.x - pos.x, m.y - pos.y };

		// If mouse is exactly on player, fall back to movement dir or default right
		if (dir.x == 0.0f && dir.y == 0.0f) {
			dir = moveDirNorm;
			if (dir.x == 0.0f && dir.y == 0.0f) dir = { 1.0f, 0.0f };
		}
		else {
			AEVec2Normalize(&dir, &dir);
		}


		AttackHitboxConfig cfg{};
		cfg.owner = this;
		cfg.lifetime = 0.30f;

		// Tentative version: circle hitbox that follows player for demo ig
		cfg.colliderShape = COL_CIRCLE;
		cfg.colliderSize = { 50.0f, 50.0f };   // diameter
		cfg.renderScale = { 50.0f, 50.0f };

		// Offset slightly in front of the player so it feels directional
		cfg.offset = { dir.x * 22.0f, dir.y * 22.0f };

		cfg.followOwner = true;
		cfg.disableOnHit = false;
		cfg.onHit = &OnMeleeHit;

		hb->Start(cfg);
		break;
	}

	case AttackType::Stab:
		// TODO: spawn forward stab hitbox GO based on moveDirNorm or mouse direction
		break;

	case AttackType::CircleAOE:
		// TODO: spawn AoE hitbox GO or do radial query for enemies
		break;

	default:
		attackCooldownTimer = 0.0f;
		break;
	}
}

void Player::TryPickup(const PickupPayload& payload)
{
	switch (payload.type)
	{
	case DropType::Ammo:
		std::cout << "Picked up ammo " << payload.amount << '\n';
		mInventory.AddAmmo(payload.amount);
		break;

	case DropType::Heal:
		std::cout << "Picked up healing " << payload.amount << '\n';
		Heal((float)payload.amount);
		break;

	case DropType::Equipment:
		if (payload.equipment)
		{
			std::cout << "Picked up equipment " << payload.equipment->name << '\n';
			mInventory.AddEquipment(payload.equipment);

			// Auto-equip so the held slot can actually attack
			mInventory.Equip(payload.equipment);

			RecalculateStats();
		}
		break;

	case DropType::Coin:
		std::cout << "Picked up coin(s) " << payload.amount << '\n';
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
