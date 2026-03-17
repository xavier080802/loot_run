#ifndef _PET_3_
#define _PET_3_
#include "Pet.h"

/*	Lycan

*/
class Pet_3 : public Pet
{
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_3; }

	bool DoSkill(const Pets::SkillCastData& _data) override;
};

#endif // !_PET_3_
