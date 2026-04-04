#ifndef _TUTORIAL_DATA_H_
#define _TUTORIAL_DATA_H_
#include "../../Pets/Pet.h"
#include "../../GameObjects/LootChest.h"
#include "../../Actor/ActorSubscriptions.h"
#include "TileMap.h"
#include "Map.h"

/*	Tutorial Sequence
	
	1. START: Fairy speaks -> Next stage
	2. MOVEMENT: Fairy speaks -> Player presses all of WASD -> Next
	3. DODGE: Fairy moves -> Speaks -> Player presses dodge key (AFTER dialogue) -> Next
	4. MELEE: Fairy moves to the wall -> Speaks -> Wall disappears and fairy moves -> Player kills enemy -> Next
	5. LOOT: Fairy moves -> Speaks -> Player opens chest (any time during this stage) -> Next
	6. RANGE: Fairy moves to wall -> Speaks -> Player holds bow -> Next
	7. BOSS: Fairy moves to wall -> Speaks -> Wall disappears -> Player kills boss
	8. END: Fairy moves -> Player enters the door -> Ends
*/

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

	class TutorialFairy : public Pet, LootChestOpenedSub, public ActorGotKillSub {
	public:
		void InitTutorial(Player* _player, TileMap& _map, MapData& _mapData);
		void Update(double dt) override;

		TutorialData data{};
		Player* player{};
		TileMap* tilemap{ nullptr }; 
		MapData* mapData{ nullptr };
		void ChangeStage(TUT_STAGE next);

		//Subscription
		void SubscriptionAlert(LootChestSubContent content) override;
		void SubscriptionAlert(ActorGotKillSubContent content) override;

		//Barrier to keep player in the room
		void SetTutBarrier();

	private:
		bool DoDialogue(float dt);
	};
}

#endif // !_TUTORIAL_DATA_H_


