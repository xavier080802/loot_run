#include "CSV.h"
#include <fstream>
#include <iostream>

CSV::CSV(std::string filename)
{
	std::ifstream file{ filename };
	if (!file.is_open()) {
		std::cout << "Failed to open file " << filename << "\n";
		return;
	}

	std::string str{};
	unsigned currCol{};
	//Get each line (row)
	while (std::getline(file, str)) {
		//New row (add new vector for the row)
		data.push_back({});
		// Tokenize to get columns
		size_t posStart{}, posEnd{};
		//Finds position of first comma, [posStart, posEnd-1] is 1 value.
		//"A,B" -> "A" and "B"
		//"A,B," -> "A", "B" and ""
		while ((posEnd = str.find_first_of(',', posStart)) != std::string::npos) {
			//Add value(column) to row. end-start is length of value
			data[rows].push_back(str.substr(posStart, posEnd - posStart));
			posStart = posEnd + 1; //+1 to ignore the leading comma
			//Increment column (to follow largest column count)
			cols = (++currCol > cols ? currCol : cols);
		}
		//Last element of row (no trailing comma)
		if (posEnd != str.size()) {
			data[rows].push_back(str.substr(posStart, std::string::npos));
			//Increment column (to follow largest column count)
			cols = (++currCol > cols ? currCol : cols);
		}
		//Row parsed.
		++rows;
		currCol = 0;
	}
	file.close();
}
