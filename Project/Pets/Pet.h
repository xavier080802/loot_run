#ifndef PET_H_
#define PET_H_

#include "AEEngine.h"
#include <string>
#include <queue>
#include "../GameObjects/GameObject.h"
#include "PetConsts.h"
#include "../Actor/StatusEffect.h"
#include <initializer_list>
#include "../GameObjects/Pathfinder.h"
class TileMap;

/*
	Base class for a Pet.
	 - Handles the player-following behavior, pet identity and cooldowns.
	 - Derived classes only control the unique features of a pet, like their skill effects
	   and unique GO_TYPE.
	 - Pet object is created by GO Manager, therefore it needs a GO_TYPE for each PET_TYPE

	 Pet scalings
	  - Read from JSON
	  - Multipliers and passive mods are scaled by rarity
*/
class Pet : public GameObject, public Pathfinder
{
public:
	//If possible, casts the pet's skill.
	//Sets the cooldown if the skill casted successfully.
	void CastSkill(const Pets::SkillCastData& data);

	//To be overrided.
	//If true, the skill icon should display the recast version
	virtual bool ShowRecastUI() const;

	//To be overrided. Called when game is starting. Used for eg. subscribing
	virtual void Setup(Player& player);

	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer) override;

	void SetTilemap(TileMap const& map);

	//Update pet logic (like skill cooldowns)
	void Update(double dt) override;

	//[0] being the start of the new path.
	//Set append to true to add to existing path instead of clearing.
	void SetPath(std::initializer_list<AEVec2> const& _path, bool append = false);

	void SetData(const Pets::PetData& newData, Pets::PET_RANK rank);
	const Pets::PetData& GetPetData();
	Pets::PET_RANK GetRank() const { return rank; }
	//Get specific multiplier from pet data
	StatEffects::Mod const& GetMultiplier(unsigned index = 0);
	//Get specific skill element from pet data
	Elements::ELEMENT_TYPE GetSkillElement(unsigned index = 0);
	bool IsOnCooldown() const { return cooldownTimer > 0.f; }
	float GetCDTimer() const { return cooldownTimer; }
	//Whether the pet has a skill that can be used (Does not check cooldown)
	bool HasSkill() const { return rank >= Pets::PET_RANK::COMMON; } //TEMP, should be >= epic

	void ClearPath();
	//Reset game-time variables
	void Reset();

	virtual ~Pet() {};

	//Whether the pet can be used or not
	bool isSet{ false };
protected:
	virtual void DoMovement(double dt);

	void MoveToTarget(double dt);
	
	//To be overrided. Get GO_TYPE of this pet.
	virtual GO_TYPE GetPetGOType() const { return GO_TYPE::PET_6; }
	//To be overrided. Performs the action of the skill.
	virtual bool DoSkill(const Pets::SkillCastData& _data);
	//To be overrided. Update function for skill behavior (if any)
	virtual void SkillUpdate(float dt);

	Pets::PetData data{};
	Pets::PET_RANK rank{};

	//Time before can cast skill again. Ticks down
	float cooldownTimer{};

	//Movement
	AEVec2 targetPos{};
	bool followPlayer{ true };
	TileMap const* tilemap{};
};

#endif // PET_H_

