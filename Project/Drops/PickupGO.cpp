#include "PickupGO.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/BitmaskUtils.h"  // adjust include path to match your project

static const GO_TYPE PICKUP_TYPE_FALLBACK = GO_TYPE::NONE;

PickupGO* PickupGO::Spawn(const AEVec2& worldPos, const PickupPayload& payload)
{
    PickupGO* p = new PickupGO();
    p->goType = PICKUP_TYPE_FALLBACK;

    // Pickups only need to collide with Player.
    // Using PET as a temporary "pickup layer" to avoid editing the enum.
    Bitmask collideWithPlayer = CreateBitmask(1, (int)Collision::LAYER::PLAYER);

    p->Init(
        worldPos,
        AEVec2{ 10.0f, 10.0f },
        0,
        MESH_SQUARE,
        Collision::SHAPE::COL_CIRCLE,
        AEVec2{ 32.0f, 32.0f },                 // tweak pickup radius/size
        collideWithPlayer,                      // can collide with Player
        Collision::LAYER::INTERACTABLE
    );
    p->goType = GO_TYPE::ITEM;
    p->InitPickup(payload);
    return p;
}

void PickupGO::InitPickup(const PickupPayload& payload)
{
    mPayload = payload;
}

void PickupGO::Update(double dt)
{
    GameObject::Update(dt);
}

void PickupGO::OnCollide(CollisionData& other)
{
    // If player touched it, disable pickup. Player will read payload + consume it.
    if (other.other.GetGOType() == GO_TYPE::PLAYER)
    {
        isEnabled = false;
        collisionEnabled = false;
    }
}