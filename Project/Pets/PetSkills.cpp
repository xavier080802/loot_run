#include "PetSkills.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include "../Actor/Player.h"
#include "../Helpers/Vec2Utils.h"
#include "Pet.h"
#include "PetManager.h"
#include <iostream>

bool PetSkills::Pet1Skill(const SkillCastData& data)
{
    //Fire projectile
    Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
    if (!proj) return false;

    AEVec2 mouse{ GetMouseWorldVec() };
    proj->Fire(data.player, {mouse.x - data.thisPet->GetPos().x, mouse.y - data.thisPet->GetPos().y},
        10, 200, 10, 
        //On collide callback: Damage enemy
        [](GameObject::CollisionData& other, Actor* caster) {
            if (other.other.GetGOType() != GO_TYPE::ENEMY) return;
            Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
            if (!pet) return;

            Actor& target = dynamic_cast<Actor&>(other.other);
            
            //Get multiplier from data and calc base damage
            target.TakeDamage(pet->GetMultiplier(0).GetValFromActor(*caster) + pet->GetMultiplier(1).GetValFromActor(*caster));
        });

    proj->SetPos(data.thisPet->GetPos());

    return true;
}

bool PetSkills::PetNullSkill(const SkillCastData& data)
{
    (void)data;
    std::cout << "PET NULL SKILL - Did PetData load correctly?\n";
    return false;
}
