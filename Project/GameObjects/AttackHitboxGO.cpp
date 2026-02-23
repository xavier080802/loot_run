#include "AttackHitboxGO.h"
#include "../Helpers/ColorUtils.h"
AttackHitboxGO* AttackHitboxGO::Start(const AttackHitboxConfig& cfg)
{
    owner = cfg.owner;
    lifespan = cfg.lifetime;
    ticks = 0;
    offset = cfg.offset;
    followOwner = cfg.followOwner;
    disableOnHit = cfg.disableOnHit;
    OnHit = cfg.onHit;
    OnEnd = cfg.onEnd;
    hitCooldown = cfg.hitCooldown;
    hitTimer = 0.f;

	hitOnce.clear();
	collisionEnabled = true;

    if (!owner)
    {
        isEnabled = false;
        return this;
    }

    AEVec2 spawnPos = owner->GetPos();
    spawnPos.x += offset.x;
    spawnPos.y += offset.y;

    Init(
        spawnPos,
        cfg.renderScale,
        cfg.zIndex,
        MESH_CIRCLE,
        cfg.colliderShape,
        cfg.colliderSize,
        owner->GetCollisionLayers(),
        owner->GetColliderLayer()
    );

	GetRenderData().tint = cfg.tint;
    goType = GO_TYPE::ATTACK_HITBOX;
    return this;
}

void AttackHitboxGO::Update(double dt)
{
    lifespan -= (float)dt;

    // Reposition hitbox to match the owner's updated position if followOwner is true
    if (owner && followOwner)
    {
        AEVec2 p = owner->GetPos();
        p.x += offset.x;
        p.y += offset.y;
        pos = p;
    }

    //If hit timer reaches cooldown (unless -1), can query collision again.
    // This allows persisting AoE hitboxes to tick damage over time
    if (hitCooldown != -1.f) {
        hitTimer += (float)dt;
        if (hitTimer >= hitCooldown) {
            hitOnce.clear(); // Clear memory of hit targets so they can be hit again
            hitTimer = 0.f;
        }
    }

    //Check ticks in event that lifespan < dt (otherwise it wont have an opportunity to collide)
    if (lifespan <= 0.0f && ticks > 0)
    {
        isEnabled = false;
        if (OnEnd) OnEnd(owner);
    }
    ++ticks;
}

void AttackHitboxGO::OnCollide(CollisionData& other)
{
	if (!owner) return;

    // Already hit this enemy during this hitbox lifetime? ignore
    for (GameObject* g : hitOnce) {
        if (g == &other.other) { return; }
    }

    // Record that we hit this object so we don't hit it again in the same swing/cooldown period
    hitOnce.push_back(&other.other);

    // Trigger the custom hit logic defined in the config
    if (OnHit) OnHit(other, owner);

    // If this was a single-target strike, immediately turn off the hitbox
    if (disableOnHit) {
        isEnabled = false;
        if (OnEnd) OnEnd(owner);
    }
}
