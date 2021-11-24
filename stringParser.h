#pragma once

#define SAME(x) x == 0
#define NOT_SAME(x) x != 0

class CStringParser {

public:

	CStringParser(const WCHAR* fileName);
	~CStringParser();

	void setNameSpace(const WCHAR* nameSpace);
	void initNameSpace();

	WCHAR* getValueByKey(WCHAR* buf, const WCHAR* key);

private:

	WCHAR* _fileName;
	WCHAR* _nameSpace;
	WCHAR* _data;
	WCHAR* _dataEnd;

	void readFile();

	WCHAR* getNextWord(WCHAR* buf, WCHAR* startPoint);
	WCHAR* skipBlank(WCHAR* startPoint);
	WCHAR* findNameSpace(WCHAR* startPoint);


};

