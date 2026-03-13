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

/**
 * @brief Sets up the actor's stats and gives it full health to start the game.
 *
 * Called as the last step of initializing either a Player or Enemy before they appear in the world.
 * After this runs, the actor's HP is full and their stats are locked in.
 *
 * @param finalStats  The ready-to-use stat block (after gear, upgrades, etc are already applied).
 *                    Passed by CONST REFERENCE. Copy the values in, doesn't store the reference.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - after RecalculateStats() is done.
 *   - Enemy::InitEnemyRuntime() - right after the EnemyDef's baseStats are loaded.
 */
void Actor::InitActorRuntime(const ActorStats& finalStats)
{
    mStats = finalStats;
    mCurrentHP = mStats.maxHP;
    mShieldValue = 0.f;
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

/**
 * @brief Makes this actor deal damage to another actor.
 *
 * This is the standard way to kick off an attack. Before the damage hits,
 * any "before dealing damage" effects (like a sword enchantment or a buff) 
 * get a chance to modify the final number. Then it calls TakeDamage on the target.
 *
 * Friendly fire is NOT handled here. That's up to the caller (usually Combat::ExecuteAttack)
 * to filter out invalid targets before calling this.
 *
 * @param target    The actor getting hit. Passed as a RAW POINTER because we need to call
 *                  methods on it, but we don't transfer ownership.
 * @param baseDmg   The initial damage number before any modifiers. Passed by VALUE (float).
 * @param dmgType   What kind of damage this is (Physical, Elemental, etc). Passed by VALUE (enum).
 * @param weapon    The weapon being used, if any (can be nullptr for unarmed or special attacks).
 *                  Passed as CONST POINTER because we only read it for callback notifications.
 *
 * @note Called by:
 *   - Combat::OnProjectileHit() - when a fired projectile connects with a target.
 *   - Combat::OnMeleeHit() - when a melee hitbox overlaps a target.
 */
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

/**
 * @brief Applies incoming damage to this actor's HP.
 *
 * This is the "receiving end" of an attack. It:
 * 1. Skips if damage is zero or negative (can't heal by hitting).
 * 2. Reduces physical/magical damage based on the actor's defense stat.
 *    (True Damage and Elemental damage skip defense entirely.)
 * 3. Subtracts the final amount from HP.
 * 4. Notifies any on-hit subscribers (for lifesteal, reactive effects etc).
 * 5. Shows a floating damage number above the actor.
 * 6. Calls OnDeath() if HP drops to zero.
 *
 * @param data  A bundle of info about the attack (amount, attacker, damage type, weapon).
 *              Passed by CONST REFERENCE. Only read from it, never modify it,
 *              and it could be a bit expensive to copy every hit.
 *
 * @note Called by:
 *   - Actor::DealDamage() - always. The attacker calls this on the target.
 */
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

    //Dmg blocked by shield - extra dmg still goes to hp
    float shieldMitigation{ min(mShieldValue, actualDmg) };
    mCurrentHP -= actualDmg - shieldMitigation;
    mShieldValue -= max(shieldMitigation, 0.f); //Deduct shield

    if (goType == GO_TYPE::PLAYER) {
        std::cout << "[Player] Took " << actualDmg << " damage. Current HP: " << mCurrentHP << "/" << mStats.maxHP << '\n';
    }
    
    // Alert on-hit subscribers (e.g., target's defensive reactive effects, or attacker's lifesteal)
    for (ActorOnHitSub* sub : onHitSubs) {
        if (!sub) continue;
        sub->SubscriptionAlert({ data.attacker, this, data.weapon, data.dmgType, actualDmg, shieldMitigation });
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

void Actor::AddShield(float value)
{
    mShieldValue += max(0, value);
}

/**
 * @brief Restores a chunk of the actor's HP without going over their maximum.
 *
 * Simple healing function. The amount is capped so that healing can never
 * push HP above the actor's max. Does nothing if given zero or negative heal.
 *
 * @param amt  How many HP points to restore. Passed by VALUE (float).
 *
 * @note Called by:
 *   - Player::TryPickup() - when the item type is Heal.
 *   - Any status effect or ability that heals in the future.
 */
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
        statusEffectsDict[eff->GetName()]->OnReapply(eff->GetStackCount());

        for (auto& sub : seGainedSubs) {
            sub->SubscriptionAlert({true, eff->GetStackCount(), *statusEffectsDict[eff->GetName()]});
        }

        delete eff;
    }
    else { //New, insert
        //Check for reaction
        if (Elements::CheckReaction(this, caster, statusEffectsDict, eff)) {
            return; //Reaction handled.
        }
        statusEffectsDict.insert(std::pair<std::string, StatEffects::StatusEffect*>(eff->GetName(), eff));
        eff->OnApply(this, caster);

        for (auto& sub : seGainedSubs) {
            sub->SubscriptionAlert({ false, eff->GetStackCount(), *statusEffectsDict[eff->GetName()] });
        }
    }
}

void Actor::UpdateStatusEffects(double dt)
{
    //Using reverse iterators cuz the status effect might be removed from the list, which breaks a normal loop.
    for (std::map<std::string, StatEffects::StatusEffect*>::reverse_iterator rit = statusEffectsDict.rbegin(); rit != statusEffectsDict.rend(); ++rit) {
        //Update effect
        rit->second->Tick(dt);
        if (statusEffectsDict.empty()) break; //This can happen if SE causes death.
        
        //Remove effect if it has ended
        if (!rit->second->IsEnded()) continue;
        delete rit->second;
        statusEffectsDict.erase(rit->first);

        //Prevent error if this is the last element
        if (statusEffectsDict.empty()) break;
    }
}

float Actor::GetStatEffectValue(STAT_TYPE stat, float baseStat)
{
    float final{};
    for (auto& pair : statusEffectsDict) {
        final += pair.second->GetFinalModVal(stat, baseStat);
    }
    return final;
}

std::map<std::string, StatEffects::StatusEffect*> const& Actor::GetStatusEffects() const
{
    return statusEffectsDict;
}

/**
 * @brief Handles the actor's death. Turns them off and broadcasts the event.
 *
 * The base version just disables the actor immediately.
 * Both Player and Enemy override this with their own extra logic:
 * - Enemy::OnDeath() also spawns drops via DropSystem.
 * - Player::OnDeath() would handle respawning or game over logic (WIP).
 *
 * @param killer  Who killed this actor, if anyone. Can be nullptr if they died from
 *                a non-attacker source. Passed as a normal POINTER.
 *
 * @note Overridden by:
 *   - Enemy::OnDeath() - enemy-specific death (dropping loot, etc).
 *   - Player::OnDeath() - player-specific death (WIP).
 */
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

void Actor::SubToSEGain(ActorGainedStatusEffectSub* sub, bool remove)
{
    for (auto it = seGainedSubs.begin(); it != seGainedSubs.end(); ++it) {
        if (*it == sub) {
            if (remove) seGainedSubs.erase(it);
            return;
        }
    }
    if (remove) return;
    seGainedSubs.push_back(sub);
}
