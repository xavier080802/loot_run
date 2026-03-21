#include "PickupGO.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/BitmaskUtils.h"
#include "../Inventory/EquipmentTypes.h"

static const GO_TYPE PICKUP_TYPE_FALLBACK = GO_TYPE::NONE;

/**
 * @brief Spawns a physical pickup item at a given world position with the specified loot data.
 *
 * This is the factory function for any item that lands on the ground.
 * It creates a new PickupGO, sets it up with a collision circle sized for easy pickup,
 * assigns only the Player collision layer so other things don't accidentally trigger it,
 * then attaches the payload (what the item actually IS).
 *
 * @param worldPos  Where in the game world to place the item. Passed by CONST REFERENCE (read the coordinates from).
 * @param payload   The item's content (coin amount, equipment pointer, etc).
 *                  Passed by CONST REFERENCE (copy its data into the pickup).
 *
 * @return A pointer to the newly created PickupGO in the world.
 *
 * @note Called by:
 *   - DropSystem::SpawnDrops() - when an enemy dies and its loot table rolls succeed.
 *   - Player::Update() [drop logic] - when the player throws away their equipped weapon with [G].
 */
PickupGO* PickupGO::Spawn(const AEVec2& worldPos, const PickupPayload& payload)
{
    PickupGO* p = new PickupGO();
    p->goType = PICKUP_TYPE_FALLBACK;

    // Pickups only need to collide with Player.
    // Using PET as a temporary "pickup layer" to avoid editing the enum.
    Bitmask collideWithPlayer = CreateBitmask(1, (int)Collision::LAYER::PLAYER);

    p->Init(
        worldPos,
        AEVec2{ 50.0f, 50.0f },
        0,
        MESH_SQUARE,
        Collision::SHAPE::COL_CIRCLE,
        AEVec2{ 32.0f, 32.0f },                 // tweak pickup radius/size
        collideWithPlayer,                      // can collide with Player
        Collision::LAYER::INTERACTABLE
    );
    p->goType = GO_TYPE::ITEM;
    p->InitPickup(payload);

    // Apply sprite + rarity tint for equipment drops
    if (payload.type == DropType::Equipment && payload.equipment) {
        const EquipmentData* eq = payload.equipment;
        if (eq->texturePath && eq->texturePath[0] != '\0') {
            p->GetRenderData().AddTexture(eq->texturePath);
        }
        p->GetRenderData().tint = GetRarityColor(eq->rarity);
    }

    return p;
}

/**
 * @brief Stores the payload into this pickup object so it can be read on pickup.
 *
 * This is an internal setup step called from Spawn(). It copies the payload by value
 * so the pickup owns its own description of what it holds.
 *
 * @param payload  What this pickup contains. Passed by CONST REFERENCE.
 */
void PickupGO::InitPickup(const PickupPayload& payload)
{
    mPayload = payload;
}

void PickupGO::Update(double dt)
{
    GameObject::Update(dt);
}

//No longer used
/*
void PickupGO::OnCollide(CollisionData& other)
{
    // If player touched it, disable pickup. Player will read payload + consume it.
    // if (other.other.GetGOType() == GO_TYPE::PLAYER)
    // {
    //     isEnabled = false;
    //     collisionEnabled = false;
    // }
    // Collision logic and item consumption is now handled entirely by the Player script.
}
*/