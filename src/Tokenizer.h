#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <string>
#include <vector>

class Tokenizer
{
public:
	Tokenizer(char separator) : _sep(separator) {}
	~Tokenizer() {}
	std::vector<const char*> tokenize(const char* line, int tokens) const
	{
		int nToks = 0;
		size_t line_len = strlen(line);
		std::vector<const char*> markers;
		const char* endOfLine = line + line_len;
		char* currMarker = line;

		markers.push_back(currMarker);
		for(int i=0;i<tokens-1;i++)
		{
			while(currMarker!=_sep)
				++currMarker;
			markers.push_back(currMarker+1);
		}

		return markers;
	}

private:
	char _sep;
};


#endif
