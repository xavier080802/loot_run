#pragma once
#include "AEEngine.h"
#include "AEMath.h"
#include "../GameObjects/GameObject.h"
#include "StatsTypes.h"
#include "StatusEffect.h"
#include <map>

class Actor : public GameObject
{
public:
	virtual ~Actor() {}

	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayer) override;
	void InitActorRuntime(const ActorStats& finalStats);
	void Free() override;

	void Update(double dt) override;

	void TakeDamage(float dmg);
	void Heal(float amt);

	bool IsDead() const { return mCurrentHP <= 0.0f; }
	float GetHP() const { return mCurrentHP; }
	float GetMaxHP() const { return mStats.maxHP; }

	//Applies the status effect and calls its OnApply.
	//Expecting the StatusEffect to be new'd
	void ApplyStatusEffect(StatEffects::StatusEffect* eff, Actor* caster);
	void UpdateStatusEffects(double dt);
	//For the given stat type, compiles the StatusEffect Mod effects based on the baseStat.
	//Final value excludes base stat amount.
	float GetStatEffectValue(STAT_TYPE stat, float baseStat);
	//Delete and clear.
	void ClearStatusEffects();

protected:
	virtual void OnDeath();

	ActorStats mStats{};
	float mCurrentHP = 0.0f;
	std::map<std::string, StatEffects::StatusEffect*> statusEffectsDict;
};