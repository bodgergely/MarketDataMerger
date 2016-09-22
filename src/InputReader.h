#ifndef _INPUTREADER_H
#define _INPUTREADER_H
#include <fstream>
#include <string>
#include <memory>
#include <iostream>
#include <queue>

class InputReader
{
public:
	InputReader() : _valid(true), _entriesRead(0){}
	virtual ~InputReader(){}
	bool isValid() const {return _valid;}
	virtual bool readLine(std::string&) = 0;
	virtual unsigned int numOfEntriesRead() const {return _entriesRead;}
protected:
	bool _valid;
	unsigned int  _entriesRead;
};



class MockInputReader : public InputReader
{
public:
	MockInputReader(const std::vector<std::string> lines) : InputReader()
	{
		for(const std::string line : lines)
			_entries.push(line);
	}
	~MockInputReader() {}

	void addLine(const std::string& line)
	{
		_entries.push(line);
	}

	virtual bool readLine(std::string& line)
	{
		if(_entries.size()>0)
		{
			line = _entries.front();
			_entries.pop();
			_entriesRead++;
			return true;
		}
		else
		{
			_valid = false;
			return false;
		}
	}

	uint	size() const {return _entries.size();}


private:
	std::queue<std::string> _entries;
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

using InputReaderPtr = InputReader*;




#endif


