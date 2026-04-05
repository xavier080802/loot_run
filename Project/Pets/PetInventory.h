#ifndef PETINVENTORY_H_
#define PETINVENTORY_H_

#include "PetConsts.h"
#include <map>

namespace Pets {


	void SaveInventory(std::map<int, std::map<int, int>> const& inventory);

	void LoadInventory(std::map<int, std::map<int, int>>& outMap);

}

#endif // PETINVENTORY_H_

