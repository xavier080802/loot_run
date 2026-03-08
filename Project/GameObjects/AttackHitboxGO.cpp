#include "AttackHitboxGO.h"
#include "../Helpers/ColorUtils.h"

/**
 * @brief Activates and configures this hitbox using the provided settings struct.
 *
 * This is essentially the "constructor" of the hitbox at runtime. Rather than passing dozens
 * of separate parameters, all settings are bundled into an AttackHitboxConfig struct and
 * passed in here. Call this once to "launch" the hitbox into the world.
 *
 * @param cfg  The complete configuration for this hitbox (size, lifetime, callbacks, etc).
 *             Passed by CONST REFERENCE because:
 *             - Don't want to modify the config that was passed in.
 *             - Avoids copying the whole struct just to read its values.
 *
 * @return Returns a pointer to 'this' so the caller can chain more setup calls if needed.
 *
 * @note Called by:
 *   - Combat::ExecuteAttack() in Combat.cpp - this is the main place hitboxes are created,
 *     one per attack type (SwingArc, Stab, CircleAOE).
 */
AttackHitboxGO* AttackHitboxGO::Start(const AttackHitboxConfig& cfg)
{
    owner = cfg.owner;
    element = cfg.element;
    knockback = cfg.knockback;
    lifespan = cfg.lifetime;
    ticks = 0;
    offset = cfg.offset;
    followOwner = cfg.followOwner;
    disableOnHit = cfg.disableOnHit;
    OnHit = cfg.onHit;
    OnEnd = cfg.onEnd;
    hitCooldown = cfg.hitCooldown;
    hitTimer = 0.f;
    extraData = cfg.extraData;

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

/**
 * @brief Called every frame to move the hitbox with its owner and check if it's expired.
 *
 * Every game frame, this does two things:
 * 1. If followOwner is true, it moves the hitbox to match the owner's current position.
 *    This keeps a melee swing stuck to the character as they move.
 * 2. Counts down the lifespan. When time is up (and at least one frame has passed),
 *    the hitbox turns itself off and fires the optional onEnd callback.
 *
 * @param dt  The amount of time (in seconds) that has passed since the last frame.
 *            Passed by VALUE.
 *
 * @note Called by:
 *   - GameObject::Update() (the base class loop) once per game frame while the hitbox is active.
 */
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

    // If hit timer reaches cooldown (unless -1), can query collision again.
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

/**
 * @brief Fires when the hitbox bumps into another game object.
 *
 * The collision system calls this automatically whenever the hitbox overlaps something.
 * It does a few safety checks before actually hurting anything:
 * - Has this specific target already been hit this swing? If so, skip it.
 *   This prevents the same sword swipe from damaging the same enemy 20 times per second.
 * - After checking, it calls the custom OnHit callback (usually Combat::OnMeleeHit)
 *   so the actual damage + knockback logic can run.
 * - If this was a one-shot hitbox (like a quick sword click), it immediately turns off.
 *
 * @param other  Information about the other object being collided with.
 *               Passed by REFERENCE because CollisionData includes the actual hit object,
 *               and we need to access and potentially modify things about it directly.
 *
 * @note Called by:
 *   - The collision detection system (CollisionManager or similar) automatically, once per overlap.
 */
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
    if (OnHit) OnHit(other, owner, element, knockback, extraData);

    // If this was a single-target strike, immediately turn off the hitbox
    if (disableOnHit) {
        isEnabled = false;
        if (OnEnd) OnEnd(owner);
    }
}
