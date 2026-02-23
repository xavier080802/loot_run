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
	// Deals damage to another actor, triggering BeforeDealingDmg subscribers first.
	// target: The actor being attacked.
	// baseDmg: The base damage calculated before any status effects or modifiers.
	// dmgType: The category of damage (e.g. Physical, Elemental) used for resistance checks.
	// weapon: The weapon equipped by the attacker used to deal the damage.
	void DealDamage(Actor* target, float baseDmg, DAMAGE_TYPE dmgType, const EquipmentData* weapon = nullptr);
	
	// Called when this actor receives damage.
	// dmg: The amount of incoming damage.
	// attacker: A pointer to the actor who dealt the damage.
	// dmgType: The category of damage taken.
	void TakeDamage(float dmg, Actor* attacker, DAMAGE_TYPE dmgType);
	
	// Restores the actor's HP by the specified amount, clamping at maxHP.
	void Heal(float amt);

	// Returns true if current HP has dropped to or below 0.
	bool IsDead() const { return mCurrentHP <= 0.0f; }

	const ActorStats& GetStats() const { return mStats; }
	float GetHP() const { return mCurrentHP; }
	float GetMaxHP() const { return mStats.maxHP; }

	//Applies a new status effect to this actor and calls its OnApply method.
	//Expects the StatusEffect to be dynamically allocated with 'new'.
	void ApplyStatusEffect(StatEffects::StatusEffect* eff, Actor* caster);
	
	// Ticks all active status effects and handles their expiration/removal.
	void UpdateStatusEffects(double dt);

	//For the given stat type, compiles the StatusEffect Mod effects based on the baseStat.
	//Final value excludes base stat amount.
	// Returns total modifier from all active status effects (does NOT include base stat)
	float GetStatEffectValue(STAT_TYPE stat, float baseStat);
	
	// Get the dictionary of active status effects currently affecting this actor.
	std::map<std::string, StatEffects::StatusEffect*> const& GetStatusEffects() const;
	
	// Deletes all active status effects and clears the dictionary.
	void ClearStatusEffects();

	//Subscribe to be alerted when this actor kills another actor. WIP (alert not implemented)
	void SubToGotKill(ActorGotKillSub* sub, bool remove = false);
	//Subscribe to be alerted when this actor dies. 
	void SubToOnDeath(ActorDeadSub* sub, bool remove = false);
	//Subscribe to be alerted when actor is about to cast something
	void SubToBeforeCast(ActorBeforeCastSub* sub, bool remove = false);
	//Subscribe to be alerted when actor is hit (on-hit)
	void SubToOnHit(ActorOnHitSub* sub, bool remove = false);
	//Subscribe to be alerted BEFORE actor deals damage
	void SubToBeforeDealingDmg(ActorBeforeDealingDmgSub* sub, bool remove = false);

protected:
	// Override point for Player/Enemy-specific death behavior
	virtual void OnDeath(Actor* killer = nullptr);

	ActorStats mStats{};
	float mCurrentHP = 0.0f;
	std::map<std::string, StatEffects::StatusEffect*> statusEffectsDict;

private:
	std::vector<ActorGotKillSub*> onKilledAnotherSubs;
	std::vector<ActorDeadSub*> onDeathSubs;
	std::vector<ActorBeforeCastSub*> beforeCastSubs;
	std::vector<ActorOnHitSub*> onHitSubs;
	std::vector<ActorBeforeDealingDmgSub*> beforeDealingDmgSubs;
};