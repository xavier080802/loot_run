#include "Player.h"
#include "../Helpers/Vec2Utils.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "../GameDB.h"
#include "../Elements/Element.h"
#include "../Actor/Combat.h"
#include "../Drops/DropSystem.h"
#include "../DebugTools.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../GameStateManager.h"
#include "../ShopFunctions.h"
#include <iostream>

namespace {
	const float DodgeCooldown{ 0.65f };
	const float DodgeIframeDur{ 0.2f };
	const AEVec2 HpBarSize{300,30};
	AEMtx33 hpBarTrans{};
	AEVec2 hpBarPos{};
	Color HpContainerCol{ 104, 11, 11, 255 };

	// Safe way to print weapon name (incase null pointer)
	static const char* SafeName(const EquipmentData* w)
	{
		return (w && w->name) ? w->name : "None";
	}
}

/**
 * @brief Initializes the Player GameObject with physics, collision, and rendering data.
 *
 * Sets up the player's initial transform, collision mesh, and assigns the correct GO_TYPE.
 * Also initializes the player's dodge cooldowns and health bar graphics.
 *
 * @param _pos               Starting world position. Passed by VALUE.
 * @param _scale             Size of the player. Passed by VALUE.
 * @param _z                 Z-index for rendering order. Passed by VALUE.
 * @param _meshShape         The visual mesh shape (e.g., Square). Passed by VALUE.
 * @param _colShape          The collision bounding box shape. Passed by VALUE.
 * @param _colSize           The size of the collision bounding box. Passed by VALUE.
 * @param _collideWithLayers Bitmask of which layers the player can hit. Passed by VALUE.
 * @param _isInLayers        The collision layer the player belongs to. Passed by VALUE.
 *
 * @return A POINTER to this newly initialized GameObject.
 *
 * @note Called by:
 *   - GameState::Init() - when the game level starts and spawns the hero.
 */
GameObject* Player::Init(AEVec2 _pos, AEVec2 _scale, int _z,
	MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize,
	Bitmask _collideWithLayers, Collision::LAYER _isInLayers)
{
	goType = GO_TYPE::PLAYER;
	dodgeCDTimer = dodgeIFrameTimer = 0.f;
	if (!squareMesh) {
		squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	}
	hpBarPos = { 0, AEGfxGetWinMinY() + HpBarSize.y * 0.5f + 5 };
	hpBarTrans = GetTransformMtx(hpBarPos, 0, HpBarSize);
	return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayers);
}

/**
 * @brief Sets up the player's runtime stats and starter equipment.
 *
 * Resets the inventory to a clean slate, loads the starter items defined in the JSON,
 * equips them, and runs the first stat calculation to initialize the actor's HP.
 *
 * @param baseStats  The player's core stats before any gear or upgrades.
 *                   Passed by CONST REFERENCE. We only copy the values into mBaseStats.
 *
 * @note Called by:
 *   - GameState::Init() - immediately after loading the player definition from GameDB.
 */
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

	EquipItem(invDef.weapon1,	EquipmentCategory::Melee,	[&](const EquipmentData* d) { mInventory.EquipMainWeapon(0, d); });
	EquipItem(invDef.weapon2,	EquipmentCategory::Melee,	[&](const EquipmentData* d) { mInventory.EquipMainWeapon(1, d); });
	EquipItem(invDef.bow,		EquipmentCategory::Ranged,	[&](const EquipmentData* d) { mInventory.EquipBow(d); });
	EquipItem(invDef.head,		EquipmentCategory::Head,	[&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.body,		EquipmentCategory::Body,	[&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.hands,		EquipmentCategory::Hands,	[&](const EquipmentData* d) { mInventory.EquipArmor(d); });
	EquipItem(invDef.feet,		EquipmentCategory::Feet,	[&](const EquipmentData* d) { mInventory.EquipArmor(d); });

	// Give some starter ammo so bow can shoot
	mInventory.AddAmmo(invDef.ammo);


	RecalculateStats();
	InitActorRuntime(mStats);

	InputManager::GetInstance()->SubscribeMouse(this, 1)
		.SubscribeKeyboard(this, 1)
		.Key(AEVK_Q).Key(AEVK_Z).Key(AEVK_X)
		.Key(AEVK_G).Key(AEVK_B);
}

/**
 * @brief Recalculates the player's final combat stats from all sources.
 *
 * Collects the base stats, adds the raw values from equipped gear (weapons & armor),
 * and then multiplies them by the shop upgrade bonuses. Also caps current HP 
 * if the new max HP is lower.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - initial setup.
 *   - Player::TryPickup() - when picking up new equipment.
 *   - Player::SubscriptionAlert() - when swapping or dropping weapons.
 *   - Player::ApplyShopUpgrades() - when returning from the shop.
 */
void Player::RecalculateStats()
{
	EquipmentModifiers eq = mInventory.GetEquipmentModifiers();
	UpgradeMultipliers up = mInventory.GetUpgradeMultipliers();
	mStats = StatsCalc::ComputeFinalStats(mBaseStats, eq, up);

	// Keep HP valid after stat changes
	if (mCurrentHP > mStats.maxHP) mCurrentHP = mStats.maxHP;
	if (mCurrentHP <= 0.0f) mCurrentHP = mStats.maxHP;
}

/**
 * @brief Reads the ShopFunctions singleton's current upgrade levels and
 * applies them to the player's UpgradeMultipliers, then recalculates all stats.
 *
 * Each shop upgrade level contributes +10% per level to the corresponding stat multiplier.
 * Example: 3 levels of Attack Speed upgrade → attackSpeedMult = 1.30 (30% faster attacks).
 *
 * @note Called by:
 *   - ShopState - after any buyShopUpgrade() or sellShopUpgrade() succeeds.
 */
void Player::ApplyShopUpgrades()
{
	ShopFunctions* shop = ShopFunctions::GetInstance();

	UpgradeMultipliers up;
	up.hpMult          = 1.0f + shop->getStatBonus(STAT_TYPE::MAX_HP);
	up.attackMult      = 1.0f + shop->getStatBonus(STAT_TYPE::ATT);
	up.defenseMult     = 1.0f + shop->getStatBonus(STAT_TYPE::DEF);
	up.moveSpeedMult   = 1.0f + shop->getStatBonus(STAT_TYPE::MOVE_SPD);
	up.attackSpeedMult = 1.0f + shop->getStatBonus(STAT_TYPE::ATT_SPD);

	// Store the multipliers in the inventory so RecalculateStats can pick them up
	mInventory.SetUpgradeMultipliers(up);
	RecalculateStats();
}

/**
 * @brief Main update loop for the Player.
 *
 * Handles ground item interaction logic (selling/swapping), tick down ability cooldowns,
 * and processes movement physics based on input.
 *
 * @param dt  Delta time since the last frame in seconds. Passed by VALUE.
 *
 * @note Called by:
 *   - GameStateManager / standard game loop - runs every frame.
 */
void Player::Update(double dt)
{
	if (mInteractablePickup && mInteractablePickup->IsEnabled())
	{
		const PickupPayload& payload = mInteractablePickup->GetPayload();
		if (payload.type == DropType::Equipment && payload.equipment)
		{
			if (AEInputCheckTriggered(AEVK_C))
			{
				mInventory.AddCoins(payload.equipment->sellPrice);
				Debug::stream << "Sold " << payload.equipment->name << " for " << payload.equipment->sellPrice << " coins.\n";
				mInteractablePickup->SetEnabled(false);
				mInteractablePickup->SetCollision(false);
				mInteractablePickup = nullptr;
			}
			else if (AEInputCheckTriggered(AEVK_E))
			{
				// Identify what is currently equipped in the corresponding slot
				const EquipmentData* oldEquip = nullptr;
				if (payload.equipment->slot == EquipSlot::Weapon) 
				{
					if (payload.equipment->weaponType == WeaponType::Bow) {
						oldEquip = mInventory.GetBow();
					} else {
						// For main weapons, replace the currently "held/focused" slot if possible
						oldEquip = GetHeldWeaponData();
						// If user isn't holding a main weapon (e.g. holds bow), default to slot 0
						if (!oldEquip || oldEquip->weaponType == WeaponType::Bow) oldEquip = mInventory.GetMainWeapon(0);
					}
				}
				else if (payload.equipment->slot == EquipSlot::Armor)
				{
					oldEquip = mInventory.GetArmor(payload.equipment->armorSlot);
				}

				if (oldEquip)
				{
					Debug::stream << "Swapped " << oldEquip->name << " for " << payload.equipment->name << "\n";
					
					// Re-purpose the dropped item to hold our OLD equipment so the player can pick it back up
					mInventory.ReplaceEquipment(oldEquip, payload.equipment);
					
					// Ensure active main weapon index matches our held weapon state before we recalculate
					if (heldWeapon == HeldWeapon::Weapon1) mInventory.SetActiveMainWeapon(0);
					else if (heldWeapon == HeldWeapon::Weapon2) mInventory.SetActiveMainWeapon(1);
					
					RecalculateStats();

					PickupPayload dropPayload{};
					dropPayload.type = DropType::Equipment;
					dropPayload.amount = 1;
					dropPayload.equipment = oldEquip;
					
					mInteractablePickup->InitPickup(dropPayload);
				}
			}
		}
	}
	
	// Reset interactable pickup; collision detection for this frame will re-assign it if we actually overlapped it today
	mInteractablePickup = nullptr;

	float fdt = (float)dt;
	if (attackCooldownTimer > 0.0f)
		attackCooldownTimer -= fdt;
	if (dodgeCDTimer > 0.f) {
		dodgeCDTimer -= fdt;
	}
	if (dodgeIFrameTimer > 0.f) {
		dodgeIFrameTimer -= fdt;
	}

	// Track input direction for minimap arrow (Player does the actual movement)
	HandleMovementInput(dt);
	Temp_DoVelocityMovement(dt);

	Actor::Update(dt);
}

/**
 * @brief Polls the keyboard for movement commands and applies velocity.
 *
 * Checks WASD for directional movement. Also handles the Spacebar dodge roll
 * if the cooldown is ready, applying a sudden burst of force and setting i-frames.
 * Updates the moveDirNorm vector for other systems to read.
 *
 * @param dt  Delta time since the last frame in seconds. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::Update() - every frame to parse player navigation inputs.
 */
void Player::HandleMovementInput(double dt)
{
	float fdt = (float)dt;

	AEVec2 dir{ 0.0f, 0.0f };
	if (AEInputCheckCurr('W')) dir.y += 1.0f;
	if (AEInputCheckCurr('S')) dir.y -= 1.0f;
	if (AEInputCheckCurr('A')) dir.x -= 1.0f;
	if (AEInputCheckCurr('D')) dir.x += 1.0f;
	if (dodgeCDTimer <= 0.f && AEInputCheckTriggered(AEVK_SPACE)) {
		ApplyForce(dir * 500.f);
		dodgeCDTimer = DodgeCooldown;
		dodgeIFrameTimer = DodgeIframeDur;
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

/**
 * @brief Retrieves the data of the weapon the player is currently yielding.
 *
 * Checks the 'heldWeapon' state to determine if the player is using Weapon1,
 * Weapon2, or the Bow, and pulls the corresponding struct from the inventory.
 *
 * @return A CONST POINTER to the currently active EquipmentData, or nullptr if unarmed.
 *
 * @note Called by:
 *   - Player::SubscriptionAlert() - to check attack type/ammo when LBUTTON is clicked.
 *   - Player::Update() - to know what weapon is being swapped out during ground item interaction.
 */
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

/**
 * @brief Attempts to add a generic ground pickup to the player's inventory.
 *
 * Sorts the payload by DropType. For equipment, it checks if the slots are full
 * before grabbing it. For ammo/health/coins, it adds them immediately.
 * 
 * @param payload  The contents of the pickup (type, amount, equipment pointer).
 *                 Passed by CONST REFERENCE. Read-only access to what the drop contains.
 *
 * @return true if the item was successfully picked up (consumed), false if the player's
 *         inventory for that item type was full and they couldn't take it.
 *
 * @note Called by:
 *   - Player::OnCollide() - when the player's hitbox overlaps a PickupGO.
 */
bool Player::TryPickup(const PickupPayload& payload)
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
			const EquipmentData* eq = payload.equipment;
			
			// Prevent picking up equipment if the relevant slots are already occupied
			if (eq->slot == EquipSlot::Weapon) 
			{
				if (eq->weaponType == WeaponType::Bow) {
					if (mInventory.GetBow() != nullptr) return false;
				} else {
					if (mInventory.GetMainWeapon(0) != nullptr && mInventory.GetMainWeapon(1) != nullptr) return false;
				}
			}
			else if (eq->slot == EquipSlot::Armor)
			{
				if (mInventory.GetArmor(eq->armorSlot) != nullptr) return false;
			}

			Debug::stream << "Picked up equipment " << payload.equipment->name << '\n';
			mInventory.AddEquipment(payload.equipment);

			// Auto-equip so the held slot can actually attack
			mInventory.Equip(payload.equipment);

            // Make sure the active index matches held weapon state so RecalculateStats works
            if (payload.equipment->slot == EquipSlot::Weapon)
            {
                if (payload.equipment->weaponType != WeaponType::Bow)
                {
                    if (heldWeapon == HeldWeapon::Weapon1) mInventory.SetActiveMainWeapon(0);
                    else if (heldWeapon == HeldWeapon::Weapon2) mInventory.SetActiveMainWeapon(1);
                }
                else
                {
                    if (heldWeapon == HeldWeapon::Bow) mInventory.SetActiveMainWeapon(2);
                }
            }

			RecalculateStats();
		}
		break;

	case DropType::Coin:
		Debug::stream << "Picked up coin(s) " << payload.amount << '\n';
		mInventory.AddCoins(payload.amount);
		break;
	default:
		break;
	}

	DropSystem::AddToPickupDisplay(payload);
	return true;
}

/**
 * @brief Gets the normalized vector of the direction the player is currently holding/moving.
 *
 * Used primarily for visual aids like showing the minimap arrow direction or
 * determining which way to drop an item if the player is standing still.
 *
 * @return A 2D vector of the movement direction. Passed by VALUE.
 *
 * @note Called by:
 *   - Minimap::Draw() - to rotate the player arrow icon.
 *   - Player::SubscriptionAlert() - to figure out which direction to toss a dropped item.
 */
AEVec2 Player::GetMoveDirNorm() const
{
	return moveDirNorm;
}

/**
 * @brief Receives input events that the player is subscribed to.
 *
 * Acts as the centralized input router for discrete actions:
 * - LBUTTON : Attack
 * - RBUTTON : Swap Melee Weapons
 * - G       : Drop Active Weapon
 * - Q, Z, X : Direct Weapon Hotkeys
 * - B       : Toggle Stats Menu
 *
 * @param content  A struct containing the key code and whether it was triggered/held/released.
 *                 Passed by VALUE.
 *
 * @note Called by:
 *   - InputManager - automatically fires when a watched key state changes.
 */
void Player::SubscriptionAlert(Input::InputKeyData content)
{
	switch (content.key)
	{
	case AEVK_LBUTTON: // Attack
		if (content.type == Input::INPUT_TYPE::CURR) {
			if (attackCooldownTimer > 0.0f) return;
			// Ensure weapon is equipped first before acknowledging attack attempt to avoid spam
			if (!GetHeldWeaponData()) {
				// We don't print "No weapon!" here because it would spam every frame while held
				return;
			}
			
			Debug::stream << "LMB triggered. cooldown=" << attackCooldownTimer << "\n";
			Debug::stream << "Attack pressed. Held=" << (int)heldWeapon << " weaponPtr=" << GetHeldWeaponData() << "\n";
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
		break;
	case AEVK_RBUTTON: // Weapon swap = Right Mouse (swap weapon1 <-> weapon2)
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
			mInventory.SwapMainWeapon();

			if (heldWeapon == HeldWeapon::Weapon1) heldWeapon = HeldWeapon::Weapon2;
			else if (heldWeapon == HeldWeapon::Weapon2) heldWeapon = HeldWeapon::Weapon1;

			RecalculateStats();

			Debug::stream << "Swapped. Held: " << SafeName(GetHeldWeaponData()) << "\n";
		}
		break;
	case AEVK_G: // Drop
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
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


			float dropDist = 50.0f;

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
		break;
	case AEVK_Q:
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
			heldWeapon = HeldWeapon::Bow;
			mInventory.SetActiveMainWeapon(2);
			RecalculateStats();
			Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
		}
		break;
	case AEVK_Z:
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
			heldWeapon = HeldWeapon::Weapon1;
			mInventory.SetActiveMainWeapon(0);
			RecalculateStats();
			Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
		}
		break;
	case AEVK_X:
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
			heldWeapon = HeldWeapon::Weapon2;
			mInventory.SetActiveMainWeapon(1);
			RecalculateStats();
			Debug::stream << "Held: " << SafeName(GetHeldWeaponData()) << "\n";
		}
		break;
	case AEVK_B:
		if (content.type == Input::INPUT_TYPE::TRIGGERED) {
			mShowStatsUI = !mShowStatsUI;
		}
		break;
	default:
		break;
	}
}

/**
 * @brief Handles player death sequence.
 *
 * Disables the player's collision and updates, then kicks the game state
 * back out to the MainMenuState. 
 *
 * @param killer  The actor who dealt the fatal blow. Passed as a RAW POINTER.
 *
 * @note Called by:
 *   - Actor::TakeDamage() - when the player's HP drops to 0.
 */
void Player::OnDeath(Actor* killer)
{
	Actor::OnDeath(killer);
	GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
	Debug::stream << "PLAYER DIED\n";
}

/**
 * @brief Handles physics collisions between the player and other GameObjects.
 *
 * Primarily filters for generic items (PickupGO) and triggers TryPickup.
 * Combat hitboxes (like enemy swords) do not run through this; they use overlapping
 * triggers managed by the AttackHitboxGO instead.
 *
 * @param other  Data about the collision (what object, the intersection area, etc).
 *               Passed by REFERENCE because it modifies the resolution data.
 *
 * @note Called by:
 *   - CollisionSystem / Physics tick - when the AABB of this player overlaps another object.
 */
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
			if (TryPickup(pickup->GetPayload()))
			{
				pickup->SetEnabled(false);
				pickup->SetCollision(false);
			}
			else if (pickup->GetPayload().type == DropType::Equipment) 
			{
				// If picking up failed (slot is occupied), flag it as interactable
				mInteractablePickup = pickup;
			}
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

/**
 * @brief Renders the player's Heads-Up Display (HUD) element overlay.
 *
 * Draws the HP bar, status effect icons, interaction prompts for ground items,
 * and the expanded stats/equipment menu if the 'B' key toggle is active.
 *
 * @note Called by:
 *   - GameState::Draw() - rendered in screen-space after the world draws.
 */
void Player::DrawUI() {
	AEVec2 camPos{};
	AEGfxGetCamPosition(&camPos.x, &camPos.y);

	if (mShowStatsUI) {
		// Draw Black Background Box
		AEVec2 bgPos = { camPos.x - 700.0f, camPos.y + 45.0f }; // Centered at left middle (somewhat)
		AEVec2 bgSize = { 600.0f, 520.0f }; 
		DrawTintedMesh(GetTransformMtx(bgPos, 0.0f, bgSize), squareMesh, nullptr, {0, 0, 0, 180}, 255);

		// Coin Counter in Top Left
		std::string coinText = "Coins: " + std::to_string(mInventory.GetCoins());
		DrawAEText(RenderingManager::GetInstance()->GetFont(), coinText.c_str(), { camPos.x - 780.0f, camPos.y + 280.0f }, 0.5f, {255, 215, 0, 255}, TEXT_MIDDLE_LEFT);
		// Ammo Counter
		std::string ammoText = "Ammo: " + std::to_string(mInventory.GetAmmo());
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ammoText.c_str(), { camPos.x - 780.0f, camPos.y + 250.0f }, 0.5f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT);

		// Stats & Equipment UI on the Left
		AEVec2 textPos = { camPos.x - 780.0f, camPos.y + 200.0f };
		float yLineSpc = -20.0f;
		
		DrawAEText(RenderingManager::GetInstance()->GetFont(), "--- STATS ---", textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("Max HP: " + std::to_string((int)mStats.maxHP)).c_str(), textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("Attack: " + std::to_string((int)mStats.attack)).c_str(), textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("Defense: " + std::to_string((int)mStats.defense)).c_str(), textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("Move Speed: " + std::to_string((int)mStats.moveSpeed)).c_str(), textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		
		std::string tSpd = std::to_string(mStats.attackSpeed);
		size_t spdLen = tSpd.length() > 4 ? 4 : tSpd.length();
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("Atk Speed: " + tSpd.substr(0, spdLen)).c_str(), textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc * 2.0f;
		
		DrawAEText(RenderingManager::GetInstance()->GetFont(), "--- EQUIPMENT ---", textPos, 0.4f, {255, 255, 255, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		
		auto w1 = mInventory.GetMainWeapon(0);
		auto w2 = mInventory.GetMainWeapon(1);
		auto bow = mInventory.GetBow();
		auto head = mInventory.GetArmor(ArmorSlot::Head);
		auto body = mInventory.GetArmor(ArmorSlot::Body);
		auto hands = mInventory.GetArmor(ArmorSlot::Hands);
		auto feet = mInventory.GetArmor(ArmorSlot::Feet);
		
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("WPN 1: " + std::string(w1 ? w1->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("WPN 2: " + std::string(w2 ? w2->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("BOW: " + std::string(bow ? bow->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("HEAD: " + std::string(head ? head->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("BODY: " + std::string(body ? body->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("HANDS: " + std::string(hands ? hands->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), ("FEET: " + std::string(feet ? feet->name : "None")).c_str(), textPos, 0.35f, {200, 200, 200, 255}, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
	}

	//Healthbar Container
	
	DrawTintedMesh(hpBarTrans, squareMesh, nullptr, HpContainerCol, 255);
	//Health indicator fill
	AEVec2 hpBarFillSize{ HpBarSize.x * (mCurrentHP / mStats.maxHP), HpBarSize.y };
	AEVec2 hpBarFillPos = hpBarPos;
	hpBarFillPos.x -= (HpBarSize.x - hpBarFillSize.x) * 0.5f;
	DrawTintedMesh(GetTransformMtx(hpBarFillPos, 0, hpBarFillSize),
		squareMesh, nullptr, {240, 20, 20, 255}, 255);
	//Shield value (if any)
	if (mShieldValue) {
		float shieldFill{ HpBarSize.x * min(mShieldValue / mStats.maxHP, 1.f) };
		DrawTintedMesh(GetTransformMtx(hpBarPos - AEVec2{ (HpBarSize.x - shieldFill) *0.5f,0}, 0, { shieldFill, HpBarSize.y }),
			squareMesh, nullptr, { 255, 255, 0, 255 }, 200);
	}
	//Hp Text: "curr (+shield) / max"
	DrawAEText(RenderingManager::GetInstance()->GetFont(),
		std::string{ std::to_string((int)mCurrentHP) + (mShieldValue ? (" (+" + std::to_string((int)mShieldValue)+")") : "")
		+ " / " + std::to_string((int)mStats.maxHP)}.c_str(),
		hpBarPos, HpBarSize.y / RenderingManager::GetInstance()->GetFontSize(), Color{ 0,0,0,255 }, TEXT_MIDDLE);

	//Status effects above hp bar
	DrawStatusEffectIcons(30, hpBarPos + AEVec2{0, HpBarSize.y * 0.5f +15}, 6, true, true);

	if (mInteractablePickup && mInteractablePickup->IsEnabled() && mInteractablePickup->GetPayload().equipment)
	{
		const EquipmentData* eq = mInteractablePickup->GetPayload().equipment;
		std::string nameStr = std::string(eq->name);
		std::string promptStr = "[E] Swap   [C] Sell (" + std::to_string(eq->sellPrice) + " Coins)";
		
		AEVec2 itemPos = { 0.0f, -55.0f }; // Centered below player and HP bar
		DrawAEText(RenderingManager::GetInstance()->GetFont(), nameStr.c_str(), itemPos, 0.4f, {255,255,255,255}, TEXT_MIDDLE);
		itemPos.y -= 20.0f;
		DrawAEText(RenderingManager::GetInstance()->GetFont(), promptStr.c_str(), itemPos, 0.4f, {255,255,255,255}, TEXT_MIDDLE);
	}

	//Tooltip
	for (auto it{ statusEffectsDict.rbegin() }; it != statusEffectsDict.rend(); ++it) {
		StatEffects::StatusEffect& se = *(*it).second;

		se.UpdateUI(true);
	}
}

/**
 * @brief Checks if the player is currently immune to damage.
 *
 * True if the player recently performed a dodge roll and is within their i-frames.
 *
 * @return true if invincible, false if vulnerable.
 *
 * @note Called by:
 *   - Actor::DealDamage() - checked by attackers before applying a hit.
 */
bool Player::IsInvulnerable()
{
	return dodgeIFrameTimer > 0.f;
}
