#include "PetSkills.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include "../Actor/Player.h"
#include "../Helpers/Vec2Utils.h"
#include "Pet.h"
#include <iostream>

bool PetSkills::Pet1Skill(const SkillCastData& data)
{
    //Fire projectile
    Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
    if (!proj) return false;

    AEVec2 mouse{ GetMouseWorldVec() };
    proj->Fire(data.player, {mouse.x - data.thisPet->GetPos().x, mouse.y - data.thisPet->GetPos().y},
        50, 200, 10, 
        //On collide callback: Damage enemy
        [](GameObject::CollisionData& other, Actor* caster) {
            if (other.other.GetGOType() != GO_TYPE::ENEMY) return;
            Actor& target = dynamic_cast<Actor&>(other.other);
            target.TakeDamage(25);
        });

    proj->SetPos(data.thisPet->GetPos());

    std::cout << "PET1 SKILL CASTED\n";
    return true;
}
