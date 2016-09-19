#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <string>
#include <vector>

class Tokenizer
{
public:
	Tokenizer(char separator) : _sep(separator) {}
	~Tokenizer() {}
	std::vector<std::string> tokenize(const std::string& line) const
	{
		size_t pos = 0;
		std::vector<std::string> tokenz;
		while(pos < line.size())
		{
			size_t fi = pos;
			while(fi<line.size() && line[fi]!=_sep)
				++fi;
			tokenz.push_back(line.substr(pos, fi-pos));
			pos = fi+1;
		}
		return tokenz;
	}

private:
	char _sep;
};


#endif
