#ifndef _CSV_H_
#define _CSV_H_
#include <string>
#include <vector>

class CSV
{
public:
	//Create CSV from a file.
	//Value format should be A,B,C\nD,E,F - No ',' between rows
	CSV(std::string filename);

	//Create CSV with data, writing to the file and initializing members
	CSV(std::string filename, std::vector<std::vector<std::string>>const& newData);

	CSV() = delete; //Default constructor
	CSV(CSV const&) = delete; //Copy constructor

	//Writes newData to the file used to contruct this CSV.
	//Updates CSV data based on newData and whether append is T/F.
	//Note: Assumes all rows have equal num of cols. If appending, assumes newData has same num of cols as existing data.
	//If append is false, deletes the existing content before writing the new content.
	bool Write(std::vector<std::vector<std::string>>const& newData, bool append = true);
	//Writes this CSV's data back into the file used to construct it.
	//If append is false, deletes the existing content before writing the new content.
	bool Write(bool append = false);

	//Static func to write to a csv
	static bool Write(std::string filename, std::vector<std::vector<std::string>>const& data, bool append = true);
	static bool Write(std::string filename, std::initializer_list<std::string> const& data, bool append = true);

	unsigned GetRows() const { return rows; }
	unsigned GetCols() const { return cols; }
	//Get string from given row and col. Returns empty string if invalid index
	std::string const& GetData(unsigned rowInd, unsigned colInd) const { 
		if (rowInd >= rows || colInd >= cols) return ""; //Invalid
		return data[rowInd][colInd]; 
	}
	//Get string through 1D index. Returns empty string if invalid index
	std::string const& GetData(unsigned ind1d) const {
		if (ind1d >= rows*cols) return ""; //Invalid
		return data[ind1d / cols][ind1d % rows];
	}
	//Gets raw data that is modifiable. Use at your own risk, remember to update rows and cols if modified.
	std::vector<std::vector<std::string>>& GetRawData() { return data; }

	void SetRows(unsigned row) { rows = row; };
	void SetCols(unsigned col) { cols = col; };

private:
	std::vector<std::vector<std::string>> data{};
	unsigned rows{};
	unsigned cols{};
	std::string fileName{};
};

#endif // !_CSV_H_
