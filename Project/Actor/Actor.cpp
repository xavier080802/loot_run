#include "Actor.h"
#include <algorithm>

GameObject* Actor::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayer)
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

void Actor::TakeDamage(float dmg)
{
    if (dmg <= 0.0f) return;
    mCurrentHP -= dmg;

    if (mCurrentHP <= 0.0f)
    {
        mCurrentHP = 0.0f;
        OnDeath();
    }
}

void Actor::Heal(float amt)
{
    if (amt <= 0.0f) return;
    mCurrentHP = AEClamp(mCurrentHP + amt, 0.0f, mStats.maxHP);
}

void Actor::ApplyStatusEffect(StatEffects::StatusEffect* eff, Actor* caster)
{
    //Check if existing
    if (statusEffectsDict.find(eff->GetName()) != statusEffectsDict.end()) {
        statusEffectsDict[eff->GetName()]->OnReapply();
    }
    else {
        //New, insert
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

        StatEffects::StatusEffect* s = rit->second;
        delete s;
        //"it" is pointing to after the element.
        statusEffectsDict.erase(std::next(rit).base());

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

void Actor::OnDeath()
{    
    // Disable immediately to prevent further collision or updates
    isEnabled = false;
    collisionEnabled = false;

    //Alert subscribers
    for (ActorDeadSub* s : onDeathSubs) {
        //TODO: get killer
        s->SubscriptionAlert({ nullptr, this });
    }
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
    beforeCastSubs.push_back(sub);
}
