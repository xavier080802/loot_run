#include "PetSkills.h"
#include "../GameObjects/GameObjectManager.h"
#include "../GameObjects/Projectile.h"
#include <iostream>

bool PetSkills::Pet1Skills(const SkillCastData& data)
{
    (void)data;
    std::cout << "PET1 SKILL CASTED\n";
    return true;
}
