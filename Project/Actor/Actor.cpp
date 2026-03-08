#include "Actor.h"
#include "../Elements/Element.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../camera.h"
#include "../DesignPatterns/PostOffice.h"
#include "StatsCalc.h"
#include <algorithm>
#include <iostream>
#include "../DebugTools.h"
#include "../UI/UIManager.h"

namespace {
    // Just for debugging
    const char* DmgTypeToString(DAMAGE_TYPE type) {
        switch (type) {
            case DAMAGE_TYPE::PHYSICAL: return "PHYSICAL";
            case DAMAGE_TYPE::MAGICAL: return "MAGICAL";
            case DAMAGE_TYPE::ELEMENTAL: return "ELEMENTAL";
            case DAMAGE_TYPE::TRUE_DAMAGE: return "TRUE_DAMAGE";
            default: return "UNKNOWN";
        }
    }

    Color DmgTypeToCol(DAMAGE_TYPE type) {
        switch (type) {
        case DAMAGE_TYPE::PHYSICAL: return {255, 128,0,255};
        case DAMAGE_TYPE::MAGICAL: return {0,0,255,255};
        case DAMAGE_TYPE::ELEMENTAL: return {0,204,204,255};
        case DAMAGE_TYPE::TRUE_DAMAGE: return {153,255,153,255};
        default: return { 247, 231, 0, 255 };
        }
    }

    const int NUM_SE_ICONS{ 4 };
}

GameObject* Actor::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer)
{
    ClearStatusEffects();
    return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayer);
}

void Actor::InitActorRuntime(const ActorStats& finalStats)
{
    mStats = finalStats;
    mCurrentHP = mStats.maxHP;
}

void Actor::Free()
{
    GameObject::Free();
    ClearStatusEffects();
}

void Actor::Update(double dt)
{
    GameObject::Update(dt);

    UpdateStatusEffects(dt);
}

void Actor::Draw()
{
    GameObject::Draw();

    float width{ scale.x / NUM_SE_ICONS }; //Width of each icon
    DrawStatusEffectIcons(width, { pos.x, pos.y - (scale.y + width) * 0.5f }, NUM_SE_ICONS);
}

void Actor::DealDamage(Actor* target, float baseDmg, DAMAGE_TYPE dmgType, const EquipmentData* weapon)
{
    // Make sure we have a valid and alive target before doing anything.
    if (!target || target->IsDead() || target->IsInvulnerable()) return;

    float finalDmg = baseDmg;

    // Trigger BeforeDealingDmg alert so that attacker's buffs or status effects 
    // can modify the finalDmg before it is applied to the target.
    for (ActorBeforeDealingDmgSub* sub : beforeDealingDmgSubs) {
        if (!sub) continue;
        sub->SubscriptionAlert({this, target, weapon, dmgType, baseDmg, finalDmg});
    }

    // Testing cout to verify DealDamage is correctly preparing the final damage
    Debug::stream << "[Actor::DealDamage] Attacker deals " << finalDmg << " "
        << DmgTypeToString(dmgType) << " damage to target!" << '\n';

    target->TakeDamage({ finalDmg, this, dmgType, weapon });
}

void Actor::TakeDamage(DamageData const& data)
{
    // Ignore non-positive damage
    if (data.dmg <= 0.0f) return;

    // Apply target defense/status effects mitigation here if necessary
    float actualDmg = data.dmg;
    
    // Physical and Magical damage are mitigated by defense.
    // True Damage and Elemental (typically) ignore defense.
    if (data.dmgType == DAMAGE_TYPE::PHYSICAL || data.dmgType == DAMAGE_TYPE::MAGICAL) {
        // Factor in any defense buffs/debuffs from active status effects
        float netDefense = mStats.defense + GetStatEffectValue(STAT_TYPE::DEF, mStats.defense);
        if (netDefense < 0.0f) netDefense = 0.0f; // Prevent negative defense increasing damage

        actualDmg = StatsCalc::ComputeDamage(data.dmg, netDefense);
    }

    // Testing cout to verify TakeDamage is receiving the attacker and damage correctly
    Debug::stream << "[Actor::TakeDamage] Target taking " << actualDmg << " damage"
              << (data.attacker ? " from attacker" : " (no attacker)")
              << " after " << mStats.defense << " defense mitigation.\n";

    mCurrentHP -= actualDmg;

    if (goType == GO_TYPE::PLAYER) {
        std::cout << "[Player] Took " << actualDmg << " damage. Current HP: " << mCurrentHP << "/" << mStats.maxHP << '\n';
    }
    
    // Alert on-hit subscribers (e.g., target's defensive reactive effects, or attacker's lifesteal)
    for (ActorOnHitSub* sub : onHitSubs) {
        if (!sub) continue;
        sub->SubscriptionAlert({ data.attacker, this, data.weapon, data.dmgType, actualDmg });
    }
    PostOffice::GetInstance()->Send("WorldTextManager", new ShowWorldTextMsg{ std::to_string((int)actualDmg),
        pos + AEVec2{static_cast<float>(rand() % 20 - 10), static_cast<float>(rand() % 20 - 10)},
        DmgTypeToCol(data.dmgType)});

    if (mCurrentHP <= 0.0f)
    {
        mCurrentHP = 0.0f;
        // Pass the attacker to OnDeath so we know who got the kill
        OnDeath(data.attacker);
    }
}

void Actor::Heal(float amt)
{
    if (amt <= 0.0f) return;
    mCurrentHP = AEClamp(mCurrentHP + amt, 0.0f, mStats.maxHP);

    Debug::stream << "HEAL: " << mCurrentHP << '\n';
}

void Actor::ApplyStatusEffect(StatEffects::StatusEffect* eff, Actor* caster)
{
    //Check if existing
    if (statusEffectsDict.find(eff->GetName()) != statusEffectsDict.end()) {
        statusEffectsDict[eff->GetName()]->OnReapply();
        delete eff;
    }
    else { //New, insert
        //Check for reaction
        if (Elements::CheckReaction(this, caster, statusEffectsDict, eff)) {
            return; //Reaction handled.
        }
        statusEffectsDict.insert(std::pair<std::string, StatEffects::StatusEffect*>(eff->GetName(), eff));
        eff->OnApply(this, caster);
    }
}

void Actor::UpdateStatusEffects(double dt)
{
    //Using reverse iterators cuz the status effect might be removed from the list, which breaks a normal loop.
    for (std::map<std::string, StatEffects::StatusEffect*>::reverse_iterator rit = statusEffectsDict.rbegin(); rit != statusEffectsDict.rend(); ++rit) {
        //Update effect
        rit->second->Tick(dt);
        
        //Remove effect if it has ended
        if (!rit->second->IsEnded()) continue;

        delete rit->second;
        //"it" is pointing to after the element.
        //statusEffectsDict.erase(std::next(rit).base());
        statusEffectsDict.erase(rit->first);

        if (statusEffectsDict.empty()) break;
    }
}

float Actor::GetStatEffectValue(STAT_TYPE stat, float baseStat)
{
    float final{ 0 };
    for (auto& pair : statusEffectsDict) {
        final += pair.second->GetFinalModVal(stat, baseStat);
    }
    return final;
}

std::map<std::string, StatEffects::StatusEffect*> const& Actor::GetStatusEffects() const
{
    return statusEffectsDict;
}

void Actor::OnDeath(Actor* killer)
{    
    // Disable immediately to prevent further collision or updates.
    isEnabled = false;
    collisionEnabled = false;

    // Alert subscribers that this actor has died.
    for (ActorDeadSub* s : onDeathSubs) {
        if (!s) continue;
        s->SubscriptionAlert({ killer, this });
    }

    // If there is a valid killer, alert the killer's subscribers that they got a kill.
    // E.g., for "on kill" buffs.
    if (killer) {
        std::cout << "[Actor::OnDeath] Actor killed by a valid attacker! Alerting killer's sub.\n";
        for (ActorGotKillSub* s : killer->onKilledAnotherSubs) {
            if (!s) continue;
            s->SubscriptionAlert({killer, this});
        }
    }
}

void Actor::DrawStatusEffectIcons(float iconSize, AEVec2 center, int numIcons, bool allowTooltip, bool isHUD) const
{
    //Draw status effects below GO.
    float displayWidth{ iconSize * numIcons };
    int num{ min(static_cast<int>(statusEffectsDict.size()), numIcons) }; //Number of SEs to show
    float pos1{ (center.x - displayWidth * 0.5f + iconSize * 0.5f) + iconSize * (float)(numIcons - num) * 0.5f }; //X-Pos of first SE. Render all icons centered
    AEGfxVertexList* mesh{ RenderingManager::GetInstance()->GetMesh(MESH_SHAPE::MESH_SQUARE) };
    int i{};
    //Render backwards, as based on EFF_TYPE, Elements appear last in the map (maps sort automatically in asc order)
    //This will show elements before other SEs (not perfect)
    for (auto it{ statusEffectsDict.rbegin() }; it != statusEffectsDict.rend(); ++it) {
        StatEffects::StatusEffect const& se = *(*it).second;
        f32 _rot = 0;
        AEVec2 _scale = { iconSize, iconSize };
        AEVec2 _pos{ pos1 + iconSize * i, center.y };
        if (!isHUD) {
            GetObjViewFromCamera(&_pos, &_rot, &_scale);
        }
        DrawMeshWithTexOffset(GetTransformMtx(_pos, _rot, _scale),
            mesh,
            RenderingManager::GetInstance()->LoadTexture(se.GetIcon().c_str()),
            { 255,255,255,255 }, renderingData->alpha,
            { 0,0 });

        if (allowTooltip && se.GetUIElement()) {
            se.GetUIElement()->ReInit(_pos, _scale, 1, Collision::COL_RECT, true, false);
        }

        //At max-visible but there's still more SEs: show a '+' to indicate more exists
        if (++i >= numIcons && statusEffectsDict.size() > numIcons) {
            _pos.x += iconSize + 2; //move to the right a bit
            DrawAEText(RenderingManager::GetInstance()->GetFont(), "+", _pos,
                (iconSize * 2) / RenderingManager::GetInstance()->GetFontSize(), { 255,255,255,255 }, TextOriginPos::TEXT_MIDDLE);
            break;
        }
    }

    //I would let them update but rn only player's ones have tooltip and calling it here will set
    //uiHovered to false before rendering the hp bar ones.
    /*for (auto it{ statusEffectsDict.rbegin() }; it != statusEffectsDict.rend(); ++it) {
        StatEffects::StatusEffect& se = *(*it).second;

        se.UpdateUI(allowTooltip);
    }*/
}

void Actor::ClearStatusEffects()
{
    for (auto& pair : statusEffectsDict) {
        delete pair.second;
    }
    statusEffectsDict.clear();
}

void Actor::SubToGotKill(ActorGotKillSub* sub, bool remove)
{
    //Check dupe
    for (auto it = onKilledAnotherSubs.begin(); it != onKilledAnotherSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) onKilledAnotherSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    onKilledAnotherSubs.push_back(sub);
}

void Actor::SubToOnDeath(ActorDeadSub* sub, bool remove)
{
    //Check dupe
    for (auto it = onDeathSubs.begin(); it != onDeathSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) onDeathSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    onDeathSubs.push_back(sub);
}

void Actor::SubToBeforeCast(ActorBeforeCastSub* sub, bool remove)
{
    //Check dupe
    for (auto it = beforeCastSubs.begin(); it != beforeCastSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) beforeCastSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    beforeCastSubs.push_back(sub);
}

void Actor::SubToOnHit(ActorOnHitSub* sub, bool remove)
{
    //Check dupe
    for (auto it = onHitSubs.begin(); it != onHitSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) onHitSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    onHitSubs.push_back(sub);
}

void Actor::SubToBeforeDealingDmg(ActorBeforeDealingDmgSub* sub, bool remove)
{
    for (auto it = beforeDealingDmgSubs.begin(); it != beforeDealingDmgSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) beforeDealingDmgSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    beforeDealingDmgSubs.push_back(sub);
}
