#ifndef _PET_INVENTORY_H_
#define _PET_INVENTORY_H_

#include "PetConsts.h"
#include <map>

namespace Pets {


	void SaveInventory(std::map<int, std::map<int, int>> const& inventory);

	void LoadInventory(std::map<int, std::map<int, int>>& outMap);

}

#endif // !_PET_INVENTORY_H_