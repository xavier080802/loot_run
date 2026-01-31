#ifndef _ACTOR_SUBSCRIPTIONS_H_
#define _ACTOR_SUBSCRIPTIONS_H_
#include "../DesignPatterns/Subscriber.h"
class Actor; //Avoid circular dependency. Do the Include in cpp.
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

#endif // !_ACTOR_SUBSCRIPTIONS_H_
