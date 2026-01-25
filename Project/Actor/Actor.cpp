#include "Actor.h"

void Actor::InitActorRuntime(const ActorStats& finalStats)
{
    mStats = finalStats;
    mCurrentHP = mStats.maxHP;
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

void Actor::OnDeath()
{
    isEnabled = false;
    collisionEnabled = false;
}