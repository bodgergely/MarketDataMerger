#ifndef _INPUTREADER_H
#define _INPUTREADER_H
#include <fstream>
#include <string>

class InputReader
{
public:
	InputReader(){}
	virtual ~InputReader(){}
	virtual std::string readLine() = 0;
};


class FileInputReader : public InputReader
{
public:
	FileInputReader(const std::string& inputFile) : _fileName(inputFile)
	{
		// TODO exception handling
		_inputStream.open(_fileName, std::ifstream::in);

	}
	~FileInputReader()
	{
		_inputStream.close();
	}

	std::string readLine()
	{
		std::string line;
        std::getline(_inputStream, line);
        return line;
	}

private:
	std::string   _fileName;
	std::ifstream _inputStream;

};


#endif


