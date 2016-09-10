#ifndef _INPUTREADER_H
#define _INPUTREADER_H
#include <fstream>
#include <string>
#include <memory>
#include <iostream>

class InputReader
{
public:
	InputReader() : _valid(true), _entriesRead(0){}
	virtual ~InputReader(){}
	bool isValid() const {return _valid;}
	virtual bool readLine(std::string&) = 0;
	unsigned int numOfEntriesRead() const {return _entriesRead;}
protected:
	bool _valid;
	unsigned int  _entriesRead;
};


class FileInputReader : public InputReader
{
public:
	FileInputReader(const std::string& inputFile) : _fileName(inputFile)
	{
		// TODO exception handling
		_inputStream.open(_fileName, std::ifstream::in);
		std::string line;
		//read and drop the first line which is the header
		std::getline(_inputStream, line);
	}
	~FileInputReader()
	{
		_inputStream.close();
	}



	bool readLine(std::string& line)
	{
        if(std::getline(_inputStream, line))
        {
        	_entriesRead++;
        	return true;
        }
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


