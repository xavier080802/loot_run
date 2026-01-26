#pragma once
#include "AEEngine.h"
#include "AEMath.h"
#include "../GameObjects/GameObject.h"
#include "StatsTypes.h"

class Actor : public GameObject
{
public:
	virtual ~Actor() {}

	void InitActorRuntime(const ActorStats& finalStats);

	void TakeDamage(float dmg);
	void Heal(float amt);

	bool IsDead() const { return mCurrentHP <= 0.0f; }
	float GetHP() const { return mCurrentHP; }
	float GetMaxHP() const { return mStats.maxHP; }

protected:
	virtual void OnDeath();

protected:
	ActorStats mStats{};
	float mCurrentHP = 0.0f;
};