#pragma once
#include "GameObject.h"
#include "../Actor/Actor.h"
#include <vector>
#include "../Inventory/EquipmentTypes.h"
#include "../Elements/Element.h"
struct AttackHitboxConfig
{
    Actor* owner = nullptr;
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE;
    float knockback = 100.0f;

    // Lifetime in seconds
    float lifetime = 0.12f;
    
    //Pending Disable (Was having bug where hitbox will not show when
    //Collide with other GO
	bool pendingDisable = false;

    // Hitbox shape + size
    Collision::SHAPE colliderShape = Collision::SHAPE::COL_CIRCLE;
    AEVec2 colliderSize = { 30.0f, 30.0f };

    // Visual size
    AEVec2 renderScale = { 30.0f, 30.0f };

    // Offset from owner position (world-space offset supplied by caller)
    AEVec2 offset = { 0.0f, 0.0f };

    // If true, the hitbox follows owner each frame
    bool followOwner = true;

    // If true, disable hitbox on first valid collision (simple “one hit” swing)
    bool disableOnHit = true;

    // Callback invoked when hitbox collides with something it can collide with
    void (*onHit)(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, void* extra) = nullptr;

    // Callback invoked when hitbox lifetime ends.
    void (*onEnd)(Actor* caster) = nullptr;

    // Time between re-hitting. If -1, hitbox can only hit each target once.
    // Cooldown is tied to the hitbox, not per enemy.
    float hitCooldown = -1;

    Color tint{ 160,160,160,180 };

    int zIndex{ 0 };

    // Ptr to whatever extra data
    void* extraData{nullptr};
};

class AttackHitboxGO : public GameObject
{
public:
    // Initializes + activates the hitbox using an explicit config struct
    AttackHitboxGO* Start(const AttackHitboxConfig& cfg);

    void Update(double dt) override;
    void OnCollide(CollisionData& other) override;

private:
    Actor* owner = nullptr;
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE;
    float knockback = 100.0f;
    float lifespan = 0.0f, hitTimer=0.0f;
    float hitCooldown{ -1 };
    int ticks{ 0 };//Number of frame ticks this has went through.
    AEVec2 offset = { 0.0f, 0.0f };
    bool followOwner = true;
    bool disableOnHit = true;
    void* extraData;

    void (*OnHit)(CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, void* extra) = nullptr;
    void (*OnEnd)(Actor* caster);
    std::vector<GameObject*> hitOnce; // enemies already damaged this swing
};

