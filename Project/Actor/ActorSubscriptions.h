#ifndef _ACTOR_SUBSCRIPTIONS_H_
#define _ACTOR_SUBSCRIPTIONS_H_
#include "../DesignPatterns/Subscriber.h"
//Avoid circular dependency. Do the Include in cpp.
class Actor; 
class EquipmentData;

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
	Actor* actor;
	EquipmentData* weapon;
	float originalDmg;
	float& finalDmg;
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
	float dmgDealt;
};
//Alert when actor is hit by something (specifically on-hit effect)
struct ActorOnHitSub : Subscriber<OnHitContent> {
	void SubscriptionAlert(OnHitContent content) override = 0;
};

//===============================================================================
#endif // !_ACTOR_SUBSCRIPTIONS_H_
