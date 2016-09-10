#ifndef _INPUTREADER_H
#define _INPUTREADER_H
#include <fstream>
#include <string>
#include <memory>

class InputReader
{
public:
	InputReader() : _valid(true){}
	virtual ~InputReader(){}
	bool isValid() const {return _valid;}
	virtual bool readLine(std::string&) = 0;
protected:
	bool _valid;
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



	bool readLine(std::string& line)
	{
        if(std::getline(_inputStream, line))
        	return true;
        else
        {
        	_valid = false;
        	return false;
        }
	}

private:
	std::string   _fileName;
	std::ifstream _inputStream;

};

using InputReaderPtr = std::shared_ptr<InputReader>;




#endif


