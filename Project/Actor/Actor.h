#pragma once
#include "AEEngine.h"
#include "AEMath.h"
#include "../GameObjects/GameObject.h"
#include "StatsTypes.h"
#include "StatusEffect.h"
#include "ActorSubscriptions.h"
#include <map>
#include <vector>

// Base class for all combat-capable entities (Player, Enemy, Boss)
class Actor : public GameObject
{
public:
	virtual ~Actor() {}

	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer) override;
	
	// Must be called after final stats are computed (base + equipment + upgrades)
	void InitActorRuntime(const ActorStats& finalStats);

	void Free() override;

	void Update(double dt) override;
	//TODO: Function to damage another actor. Do calcs and stuff, call TakeDamage, and call alerts
	void TakeDamage(float dmg);
	void Heal(float amt);

	bool IsDead() const { return mCurrentHP <= 0.0f; }

	const ActorStats& GetStats() const { return mStats; }
	float GetHP() const { return mCurrentHP; }
	float GetMaxHP() const { return mStats.maxHP; }

	//Applies the status effect and calls its OnApply.
	//Expecting the StatusEffect to be new'd
	void ApplyStatusEffect(StatEffects::StatusEffect* eff, Actor* caster);
	void UpdateStatusEffects(double dt);

	//For the given stat type, compiles the StatusEffect Mod effects based on the baseStat.
	//Final value excludes base stat amount.
	// Returns total modifier from all active status effects (does NOT include base stat)
	float GetStatEffectValue(STAT_TYPE stat, float baseStat);
	//Get the dictionary of status effects affecting this actor.
	std::map<std::string, StatEffects::StatusEffect*> const& GetStatusEffects() const;
	//Delete and clear.
	void ClearStatusEffects();

	//Subscribe to be alerted when this actor kills another actor. WIP (alert not implemented)
	void SubToGotKill(ActorGotKillSub* sub);
	//Subscribe to be alerted when this actor dies. 
	void SubToOnDeath(ActorDeadSub* sub, bool remove = false);
	//Subscribe to be alerted when actor is about to cast something
	void SubToBeforeCast(ActorBeforeCastSub* sub, bool remove = false);
	//Subscribe to be alerted when actor is hit (on-hit)
	void SubToOnHit(ActorOnHitSub* sub, bool remove = false);

protected:
	// Override point for Player/Enemy-specific death behavior
	virtual void OnDeath();

	ActorStats mStats{};
	float mCurrentHP = 0.0f;
	std::map<std::string, StatEffects::StatusEffect*> statusEffectsDict;

private:
	std::vector<ActorGotKillSub*> onKilledAnotherSubs;
	std::vector<ActorDeadSub*> onDeathSubs;
	std::vector<ActorBeforeCastSub*> beforeCastSubs;
	std::vector<ActorOnHitSub*> onHitSubs;
};