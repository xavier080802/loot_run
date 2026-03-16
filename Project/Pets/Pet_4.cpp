#include "Pet_4.h"
#include "../GameObjects/GameObjectManager.h"
#include "Whirlpool.h"
#include "../Helpers/Vec2Utils.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "PetManager.h"
#include <sstream>

namespace {
    //extra must be ptr to pet
    void PopHit(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float, void* extra) {
        Pet_4* pet{ static_cast<Pet_4*>(extra) };
        Actor* other{ dynamic_cast<Actor*>(&target.other) };
        if (!pet || !other) return;
        Pets::PetData const& data{ pet->GetPetData() };
        //Do damage
        float dmg{};
        for (StatEffects::Mod const& m : data.multipliers) {
            dmg += m.GetValFromActor(*caster);
        }
        AEVec2 poolPos{ pet->GetWhirlpool()->GetPos() }, otherPos{other->GetPos()};
        float dist{ AEVec2Distance(&poolPos, &otherPos)};
        caster->DealDamage(other, dmg * AEClamp(dist / pet->GetWhirlpool()->GetColSize().x, pet->GetMinDmgMult(), 1.f), data.dmgTypes.at(0));
        Elements::ApplyElement(element, caster, other);
    }
}

void Pet_4::Setup(Player&)
{
    pullStrength = std::stof(data.extra["pull"]);
    duration = std::stof(data.extra["duration"]);
    minDmgMult = std::stof(data.extra["minDmgFalloff"]);
    float sz = std::stof(data.extra["size"]);
    poolSize = { sz, sz };
    std::istringstream is{ data.extra["color"] };
    is >> poolCol.r >> poolCol.g >> poolCol.b >> poolCol.a;
}

bool Pet_4::DoSkill(const Pets::SkillCastData& _data)
{
    //Recast to end pool early. Then skill returns true to set real cooldown
    if (poolActive) {
        return Recast();
    }
    //Spawn whirlpool
    GameObject* go = GameObjectManager::GetInstance()->FetchGO(GO_TYPE::WHIRLPOOL);
    pool = dynamic_cast<Whirlpool*>(go);
    if (!pool) return false;

    //Place at cursor and initialize
    AEVec2 mouse{ GetMouseWorldVec() };
    pool->SetupWhirlpool(duration, pullStrength)
        .Init(mouse, poolSize, -4, MESH_CIRCLE, Collision::SHAPE::COL_CIRCLE, poolSize,
        _data.player->GetCollisionLayers(), _data.player->GetColliderLayer());
    pool->GetRenderData().tint = poolCol;
    pool->GetRenderData().alpha = poolCol.a;

    recastTimer = duration;
    poolActive = true;
    return false;
}

void Pet_4::SkillUpdate(float dt)
{
    if (!poolActive) return;

    recastTimer -= dt;
    //Pool gone, do stuff
    if (recastTimer <= 0) {
        //Set cooldown for real
        cooldownTimer = data.skillCooldown;
        Recast();
    }
}

bool Pet_4::Recast()
{
    if (!pool) return false;
    pool->SetEnabled(false);

    //Set hitbox on pool for last instance of damage
    GameObject* go = GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX);
    AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(go);
    if (!hb) return false;
    AttackHitboxConfig cfg{};
    cfg.owner = &PetManager::GetInstance()->GetPlayer();
    cfg.offset = pool->GetPos() - cfg.owner->GetPos();
    cfg.colliderSize = cfg.renderScale = poolSize;
    cfg.tint = {};
    cfg.hitCooldown = -1;
    cfg.knockback = 0;
    cfg.followOwner = cfg.disableOnHit = false;
    cfg.element = data.skillElements.at(0);
    cfg.onHit = PopHit;
    cfg.extraData = this;
    hb->Start(cfg);
    Bitmask bm{ cfg.owner->GetCollisionLayers() };
    hb->SetCollisionLayers(ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE));

    poolActive = false;
    return true;
}
