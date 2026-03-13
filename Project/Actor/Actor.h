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
	//Data of an attack
	struct DamageData {
		float dmg; //Incoming damage
		Actor* attacker; //Ptr to the actor who is dealing the dmg (if any)
		DAMAGE_TYPE dmgType; //Category of damage taken
		EquipmentData const* weapon; //Weapon used to inflict dmg (if any)
	};

	virtual ~Actor() {}

	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer) override;
	
	// Must be called after final stats are computed (base + equipment + upgrades)
	void InitActorRuntime(const ActorStats& finalStats);

	void Free() override;

	void Update(double dt) override;

	void Draw() override;

	// Deals damage to another actor, triggering BeforeDealingDmg subscribers first.
	// target: The actor being attacked.
	// baseDmg: The base damage calculated before any status effects or modifiers.
	// dmgType: The category of damage (e.g. Physical, Elemental) used for resistance checks.
	// weapon: The weapon equipped by the attacker used to deal the damage.
	virtual void DealDamage(Actor* target, float baseDmg, DAMAGE_TYPE dmgType, const EquipmentData* weapon = nullptr);
	
	// Called when this actor receives damage.
	virtual void TakeDamage(DamageData const& data);

	//Grant the actor a shield of the given value.
	void AddShield(float value);
	float GetShieldVal() const { return mShieldValue; }

	virtual bool IsInvulnerable() { return false; }
	
	// Restores the actor's HP by the specified amount, clamping at maxHP.
	void Heal(float amt);

	// Returns true if current HP has dropped to or below 0.
	bool IsDead() const { return mCurrentHP <= 0.0f; }

	const ActorStats& GetStats() const { return mStats; }
	virtual const ActorStats& GetBaseStats() const { return mStats; }
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
	//Sub to be alerted when this actor is inflicted with a status effect
	void SubToSEGain(ActorGainedStatusEffectSub* sub, bool remove = false);

protected:
	// Override point for Player/Enemy-specific death behavior
	virtual void OnDeath(Actor* killer = nullptr);

	// Draw icons of status effects
	// iconSize: Icons are square
	// center: Center position from which icons are drawn around
	// numIcons: Max number of icons to display. Display width is iconSize*numIcons
	void DrawStatusEffectIcons(float iconSize, AEVec2 center, int numIcons, bool allowTooltip = false, bool isHUD = false) const;

	ActorStats mStats{};
	float mCurrentHP = 0.0f;
	float mShieldValue{}; //Blocks post-mitigation damage from affecting hp. 
	std::map<std::string, StatEffects::StatusEffect*> statusEffectsDict;

private:
	std::vector<ActorGotKillSub*> onKilledAnotherSubs;
	std::vector<ActorDeadSub*> onDeathSubs;
	std::vector<ActorBeforeCastSub*> beforeCastSubs;
	std::vector<ActorOnHitSub*> onHitSubs;
	std::vector<ActorBeforeDealingDmgSub*> beforeDealingDmgSubs;
	std::vector<ActorGainedStatusEffectSub*> seGainedSubs;
};