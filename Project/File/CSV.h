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

	CSV() = delete; //Default constructor
	CSV(CSV const&) = delete; //Copy constructor

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
};

#endif // !_CSV_H_
