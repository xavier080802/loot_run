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
        owner->GetZ(),
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

    if (owner && followOwner)
    {
        AEVec2 p = owner->GetPos();
        p.x += offset.x;
        p.y += offset.y;
        pos = p;
    }

    //If hit timer reaches cooldown (unless -1), can query collision again.
    if (hitCooldown != -1.f) {
        hitTimer += (float)dt;
        if (hitTimer >= hitCooldown) {
            hitOnce.clear();
            hitTimer = 0.f;
        }
    }

    //Check ticks in event that lifespan < dt (otherwise it wont have an opportunity to collide)
    if (lifespan <= 0.0f && ticks > 0)
    {
        isEnabled = false;
    }
    ++ticks;
}

void AttackHitboxGO::OnCollide(CollisionData& other)
{
	if (!owner) return;

    // Only care about enemies (prevents storing random walls/items)
    if (other.other.GetGOType() != GO_TYPE::ENEMY) return;

    // Already hit this enemy during this hitbox lifetime? ignore
    for (GameObject* g : hitOnce) {
        if (g == &other.other) { return; }
    }

    hitOnce.push_back(&other.other);

    if (OnHit) OnHit(other, owner);

    if (disableOnHit)
        isEnabled = false;
}
