#include "CSV.h"
#include <fstream>
#include <iostream>

CSV::CSV(std::string filename) : fileName{filename}
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

CSV::CSV(std::string filename, std::vector<std::vector<std::string>> const& newData)
	: fileName{ filename }, data{ newData }, cols{ static_cast<unsigned>(newData.size()) }
{
	Write(newData, false);
}

bool CSV::Write(std::vector<std::vector<std::string>> const& newData, bool append)
{
	std::ofstream file{ fileName, append ? std::ios_base::app : (std::ios::out | std::ios::trunc) };
	if (!file.is_open()) {
		std::cout << "Failed to open file [" << fileName << ']' << "\n";
		return false;
	}

	unsigned _cols{};
	for (unsigned r{}; r < newData.size(); ++r) {
		for (unsigned c{}; c < newData.at(r).size(); ++c) {
			file << newData.at(r).at(c);
			if (c < newData.at(r).size() - 1) file << ',';
			_cols = (c + 1 > _cols ? c + 1 : _cols); //Count highest num of columns
		}
		file << '\n';
	}

	if (!append) {
		data = newData;
		rows = newData.size();
		cols = _cols;
	}
	else {
		data.insert(data.end(), newData.begin(), newData.end());
		rows += newData.size();
	}
	file.close();
	return true;
}

bool CSV::Write(bool append)
{
	return Write(data, append);
}

bool CSV::Write(std::string filename, std::vector<std::vector<std::string>> const& data, bool append)
{
	std::ofstream file{ filename, append ? std::ios_base::app : (std::ios::out | std::ios::trunc) };
	if (!file.is_open()) {
		std::cout << "Failed to open file [" << filename << ']' << "\n";
		return false;
	}

	for (unsigned r{}; r < data.size(); ++r) {
		for (unsigned c{}; c < data.at(r).size(); ++c) {
			file << data.at(r).at(c);
			if (c < data.at(r).size() - 1) file << ',';
		}
		file << '\n';
	}
	return true;
}

bool CSV::Write(std::string filename, std::initializer_list<std::string> const& data, bool append)
{
	std::ofstream file{ filename, append ? std::ios_base::app : (std::ios::out | std::ios::trunc) };
	if (!file.is_open()) {
		std::cout << "Failed to open file [" << filename << ']' << "\n";
		return false;
	}
	for (std::string const& str : data) {
		file << str;
		if (data.end() != (&str + 1)) file << ',';
	}
	file << '\n';
	return true;
}
