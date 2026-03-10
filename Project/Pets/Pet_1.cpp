#include "Pet_1.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include "../Actor/Player.h"
#include "../Helpers/Vec2Utils.h"
#include "PetManager.h"

bool Pet_1::DoSkill(const Pets::SkillCastData& data)
{
    //Fire projectile
    Projectile* proj = dynamic_cast<Projectile*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PROJECTILE));
    if (!proj) return false;

    AEVec2 mouse{ GetMouseWorldVec() };
    proj->Fire(data.player, { mouse.x - pos.x, mouse.y -pos.y },
        10, 200, 10,
        //On collide callback: Damage enemy
        [](GameObject::CollisionData& other, Actor* caster, Elements::ELEMENT_TYPE /*element*/, float /*knockback*/) {
            if (other.other.GetGOType() != GO_TYPE::ENEMY) return;
            Pet* pet{ PetManager::GetInstance()->GetEquippedPet() };
            if (!pet) return;

            Actor& target = dynamic_cast<Actor&>(other.other);

            //Get multiplier from data and calc base damage
            caster->DealDamage(&target, pet->GetMultiplier(0).GetValFromActor(*caster) + pet->GetMultiplier(1).GetValFromActor(*caster),
                DAMAGE_TYPE::MAGICAL, nullptr);

            //Apply element (if any)
            Elements::ApplyElement(pet->GetSkillElement(), caster, &target);
        });

    proj->SetPos(pos);

    return true;
}
