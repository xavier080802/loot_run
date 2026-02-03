#ifndef _TUTORIAL_DATA_H_
#define _TUTORIAL_DATA_H_
#include "./Pets/Pet.h"
#include "./Map.h"
#include "./GameObjects/LootChest.h"
#include "./Actor/ActorSubscriptions.h"
#include "./Actor/Enemy.h"

namespace Tutorial {
	enum TUT_STAGE {
		START,
		MOVEMENT,
		DODGE,
		MELEE,
		LOOT,
		RANGE,
		BOSS,
		END,
	};

	struct TutorialData{
		TUT_STAGE stage;
		bool isWaiting{ false };
		bool isFollowing{ false };

		//---Dialogue---
		bool playDialogue{ false };
		std::vector<std::string> dialogueLines;
		int currDialogueLine;
		AEVec2 dialoguePos{0, -200};
		float dialogueSize{ 0.5f };

		//---Used for checking stage---
		float timer;
		std::string checks{};
	};

	class TutorialFairy : public Pet, 
		LootChestOpenedSub, ActorDeadSub, ActorBeforeCastSub {
	public:
		void InitTutorial(Player* _player, MapData* _map, std::initializer_list<Enemy*> enemies);
		void Update(double dt) override;

		TutorialData data{};
		Player* player;
		MapData* map;
		void ChangeStage(TUT_STAGE next);

		//Subscription
		void SubscriptionAlert(LootChestSubContent content) override;
		void SubscriptionAlert(ActorBeforeCastContent content) override;
		void SubscriptionAlert(ActorDeadSubContent content) override;

		//Barrier to keep player in the room
		AEVec2 GetTutBarrier();

	private:
		bool DoDialogue(float dt);
	};
}

#endif // !_TUTORIAL_DATA_H_


