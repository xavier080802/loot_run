#include "Pet_1.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include "../Actor/Player.h"
#include "../Helpers/Vec2Utils.h"
#include "PetManager.h"
#include "../DesignPatterns/PostOffice.h"
#include "../GameObjects/AttackHitboxGO.h"
#include <cmath>
#include <sstream>

namespace {
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    void AOEOnHit(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE, float, EquipmentData*, void* extra) {
        Pet_1* pet{ static_cast<Pet_1*>(extra) };
        Actor* other{ dynamic_cast<Actor*>(&target.other) };
        if (!pet || !other || !caster) return;
        //Damage
        float base{ pet->GetMultiplier(2).GetValFromActor(*caster) + pet->GetMultiplier(3).GetValFromActor(*caster) };
        float max{ pet->GetMultiplier(6).GetValFromActor(*caster) + pet->GetMultiplier(7).GetValFromActor(*caster) };
        caster->DealDamage(other, lerp(base, max, pet->GetAOEChargeMult()), pet->GetPetData().dmgTypes.at(0));
    }
}

float Pet_1::CalcBasicDmg(Actor& caster)
{
    float base{ GetMultiplier(0).GetValFromActor(caster) + GetMultiplier(1).GetValFromActor(caster) };
    float max{ GetMultiplier(4).GetValFromActor(caster) + GetMultiplier(5).GetValFromActor(caster) };
    return lerp(base, max, GetChargeMult());
}

float Pet_1::GetChargeMult() const
{
    return AEClamp(chargeTimer / maxChargeDur, 0, 1);
}

float Pet_1::GetAOEChargeMult() const
{
    return AEClamp((chargeTimer - aoeChargeThreshold) / (maxChargeDur - aoeChargeThreshold), 0, 1);
}

void Pet_1::DoAOE(Actor& caster, AEVec2 impactPos)
{
    //Check charge time
    if (chargeTimer < aoeChargeThreshold) return;
    //Create the AOE
    GameObject* go = GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX);
    AttackHitboxGO* hb = dynamic_cast<AttackHitboxGO*>(go);
    if (!hb) return;
    Player& p{ PetManager::GetInstance()->GetPlayer() };
    AttackHitboxConfig cfg{};
    cfg.owner = &p;
    cfg.offset = impactPos - p.GetPos();
    cfg.colliderSize = cfg.renderScale = AEVec2{aoeDiameter, aoeDiameter};
    cfg.tint = aoeCol;
    cfg.hitCooldown = -1;
    cfg.followOwner = cfg.disableOnHit = false;
    cfg.element = data.skillElements.at(0);
    cfg.onHit = AOEOnHit;
    cfg.extraData = this;
    hb->Start(cfg);
    Bitmask bm{ cfg.owner->GetCollisionLayers() };
    hb->SetCollisionLayers(ResetFlagAtPos(&bm, Collision::LAYER::OBSTACLE));
}

void Pet_1::OnProjHit(Actor& caster, AEVec2 impactPos, bool allowAOE)
{
    returned = true;
    //Start cooldown now
    //Cooldown refund for shorter charge time
    float mult{ AEClamp(chargeTimer / maxChargeDur, 0, 1) };
    cooldownTimer = data.skillCooldown * maxCDRefund * mult;

    if (allowAOE)
        DoAOE(caster, impactPos);
}

void Pet_1::Setup(Player& player)
{
    chargeTimer = 0.f;
    charging = false;
    returned = true;

    maxChargeDur = std::stof(data.extra.at("maxChargeDur"));
    aoeChargeThreshold = std::stof(data.extra.at("aoeChargeThreshold"));
    resetChargeDur = std::stof(data.extra.at("resetChargeDur"));
    maxCDRefund = std::stof(data.extra.at("maxCDRefund"));
    aoeDiameter = std::stof(data.extra.at("aoeDiameter"));
    projSpd = std::stof(data.extra.at("projSpd"));
    projRadius = std::stof(data.extra.at("projRadius"));
    std::istringstream is{ data.extra["aoeCol"] };
    is >> aoeCol.r >> aoeCol.g >> aoeCol.b >> aoeCol.a;
}

bool Pet_1::DoSkill(const Pets::SkillCastData& _data)
{
    if (!returned) return false;

    if (!charging) {
        charging = true;
        chargeTimer = 0.f;
        PostOffice::GetInstance()->Send("WorldTextManager", new ShowWorldTextMsg{ "CHARGING", pos + AEVec2{0, scale.y * 0.5f} });
        return false;
    }
    //Fire projectile
    Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
    if (!proj) return false;

    AEVec2 mouse{ GetMouseWorldVec() };
    proj->Fire(_data.player, { mouse.x - pos.x, mouse.y -pos.y },
        projRadius, projSpd, 3,
        //On collide callback: Damage enemy
        [](GameObject::CollisionData& other, Actor* caster, Elements::ELEMENT_TYPE /*element*/, float /*knockback*/) {
            Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
            Pet_1* rock{ dynamic_cast<Pet_1*>(pet) };
            if (!rock) return;
            rock->OnProjHit(*caster, other.other.GetPos());

            if (other.other.GetGOType() != GO_TYPE::ENEMY || !caster) return;
            Actor& target = dynamic_cast<Actor&>(other.other);

            //Get multiplier from data and calc base damage
            caster->DealDamage(&target, rock->CalcBasicDmg(*caster),
                pet->GetPetData().dmgTypes.at(0), nullptr);

            //Apply element (if any)
            Elements::ApplyElement(pet->GetSkillElement(), caster, &target);
        });

    proj->SetPos(pos);

    //Set callbacks to "return" rock when proj hits tile or dies
    proj->SetHitTileCallback([](Projectile& p, bool) {
            Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
            Pet_1* rock{ dynamic_cast<Pet_1*>(pet) };
            if (!rock) return;
            rock->OnProjHit(*p.GetOwner(), p.GetPos());
        })
        .SetTimeoutCallback([](Projectile& p) {
            Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
            Pet_1* rock{ dynamic_cast<Pet_1*>(pet) };
            if (!rock) return;
            rock->OnProjHit(*p.GetOwner(), p.GetPos(), false);
        });

    charging = returned = hasDoneChargeAlert = false;
    return true;
}

void Pet_1::SkillUpdate(float dt)
{
    if (!charging) return;

    chargeTimer += dt;

    //Player did not recast. Cancel
    if (chargeTimer >= resetChargeDur) {
        charging = false;
        //Start cooldown now
        cooldownTimer = data.skillCooldown;
        return;
    }

    if (!hasDoneChargeAlert && chargeTimer >= maxChargeDur) {
        hasDoneChargeAlert = true;
        PostOffice::GetInstance()->Send("WorldTextManager", new ShowWorldTextMsg{ "FULL POWER [R]", PetManager::GetInstance()->GetPlayer().GetPos(), Color{200, 0,0, 255}, 2.5f, 30.f });
    }
    else if (std::ceilf(chargeTimer) - chargeTimer <= dt){
        PostOffice::GetInstance()->Send("WorldTextManager",
            new ShowWorldTextMsg{ "Charging", PetManager::GetInstance()->GetPlayer().GetPos(), Color{247, 231,100, 255}, 1.f, 20.f });
    }
}
