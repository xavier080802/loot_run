#ifndef _ACTOR_SUBSCRIPTIONS_H_
#define _ACTOR_SUBSCRIPTIONS_H_
#include "../DesignPatterns/Subscriber.h"
#include "StatsTypes.h"
class Actor; //Avoid circular dependency. Do the Include in cpp.
struct EquipmentData;
//Declarations for Subscribers related to Actor

struct ActorGotKillSubContent {
	Actor* killer;
	Actor* victim;
};
//Alert when actor kills another.
struct ActorGotKillSub : Subscriber<ActorGotKillSubContent> {
	void SubscriptionAlert(ActorGotKillSubContent content) override = 0;
};

//===============================================================================

struct ActorDeadSubContent {
	Actor* killer;
	Actor* victim;
};
//Alert when actor dies.
struct ActorDeadSub : Subscriber<ActorDeadSubContent> {
	void SubscriptionAlert(ActorDeadSubContent content) override = 0;
};

//===============================================================================

struct BeforeDealingDmgContent {
	Actor* attacker; // The actor performing the attack
	Actor* target; // The actor receiving the attack
	const EquipmentData* weapon; // Weapon used, if any
	DAMAGE_TYPE dmgType; // Type of damage being dealt
	float originalDmg; // Original base damage before modifiers
	float& finalDmg; // Reference to the actual dmg that will be dealt. Subs can modify this.
};
//Alert BEFORE actor does damage to victim
struct ActorBeforeDealingDmgSub : Subscriber<BeforeDealingDmgContent> {
	void SubscriptionAlert(BeforeDealingDmgContent content) override = 0;
};

//===============================================================================

struct ActorBeforeCastContent {
	//Allows subs to deny cast
	bool& allowCast;
	Actor* actor;
	const EquipmentData* weapon;
};
//Alert before actor casts something
struct ActorBeforeCastSub : Subscriber<ActorBeforeCastContent> {
	void SubscriptionAlert(ActorBeforeCastContent content) override = 0;
};

//===============================================================================\

struct OnHitContent {
	Actor* attacker, *actor;
	//Add hit details here
	const EquipmentData* weapon;
	DAMAGE_TYPE dmgType; // The type of damage that was taken
	float dmgDealt; // Exact damage successfully dealt to shield+health
	float shieldMitigated; // Damage mitigated by shield. Is included in dmgDealt
};
//Alert when actor is hit by something (specifically on-hit effect)
struct ActorOnHitSub : Subscriber<OnHitContent> {
	void SubscriptionAlert(OnHitContent content) override = 0;
};

//===============================================================================

struct EffectAppliedContent {
	bool reapplied;
	unsigned stacksApplied;
	StatEffects::StatusEffect& se;
};
struct ActorGainedStatusEffectSub : Subscriber<EffectAppliedContent> {
	void SubscriptionAlert(EffectAppliedContent content) override = 0;
};

//===============================================================================
#endif // !_ACTOR_SUBSCRIPTIONS_H_
