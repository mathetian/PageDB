#include "../include/CHash.h"
#include <sys/stat.h>

Chain::Chain(ChainHash * cHash, int defaultFirstOffset)
{
	this -> cHash = cHash;
	firstoffset = defaultFirstOffset;
}

Chain::~Chain()
{
}

bool Chain::put(const string&key,const string&value, int hashVal)
{
	if(check(key, hashVal) != 0)
		return false;

	Elem elem(firstoffset, key.size(), value.size());

    int offset = cHash -> findSuitable(key.size() + value.size() + SELEM);  
    
    cHash -> datfs.seekg(offset, ios_base::beg);
    cHash -> datfs.write((char*)&elem, SELEM);
    cHash -> datfs.write(key.c_str(), sizeof(key));
    cHash -> datfs.write(value.c_str(), sizeof(value));

    firstoffset = offset;

	return true;
}	

string   Chain::get(const string&key, int hashVal)
{
	int offset = firstoffset; Elem elem;
	while(offset != -1)
	{
		cHash -> datfs.seekg(offset, ios_base::beg);
		cHash -> datfs.read ((char*)&elem, SELEM);
		offset = elem.nextOffset; 
		if(elem.hashVal != hashVal || elem.keySize != key.size())
			continue;
		char * content = new char[elem.keySize + elem.valueSize + 2];
		cHash -> datfs.read(content, elem.keySize + elem.valueSize);
		string key1(content, content + elem.keySize);
		string value1(content + elem.keySize, content + elem.keySize + elem.valueSize);
		delete content; content = NULL;
		if(key == key1) return value1;
	}
	return "";
}

bool   Chain::check(const string&key, int hashVal)
{
	int offset = firstoffset; Elem elem;
	while(offset != -1)
	{
		cHash -> datfs.seekg(offset, ios_base::beg);
		cHash -> datfs.read ((char*)&elem, SELEM);
		offset = elem.nextOffset; 
		if(elem.hashVal != hashVal || elem.keySize != key.size())
			continue;
		char * content = new char[elem.keySize + 2];
		cHash -> datfs.read(content, elem.keySize);
		string key1(content, content + elem.keySize);
		delete content; content = NULL;
		if(key == key1) return true;
	}
	return false;
}

bool Chain::remove(const string&key, int hashVal)
{
	int offset = firstoffset; Elem elem;
	while(offset != -1)
	{
		cHash -> datfs.seekg(offset, ios_base::beg);
		cHash -> datfs.read ((char*)&elem, SELEM);
		offset = elem.nextOffset; 
		if(elem.hashVal != hashVal || elem.keySize != key.size())
			continue;
		char * content = new char[elem.keySize + 2];
		cHash -> datfs.read(content, elem.keySize);
		string key1(content, content + elem.keySize);
		delete content; content = NULL;
		if(key == key1)
		{
			/**
				Storage the emptry space into the the header linklist
			**/
			return true;
		}
	}
	return false;
}

ChainHash::ChainHash(int chainCount, HASH hashFunc)
{
	this -> chainCount = chainCount;
	this -> hashFunc = hashFunc;
}

ChainHash::~ChainHash()
{
	datfs.close(); int index = 0;
	for(;index < chainCount;index++)
	{
		delete headers[index];
		headers[index] = NULL;
	}
}

bool ChainHash::init(const string & filename)
{
   struct stat buf;
   string datFileName = filename + ".dat";

   datfs.open (datFileName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);

   if((stat(datFileName.c_str(), &buf) == -1) || buf.st_size == 0)
   { 
	    entries.push_back(0);
	    entries = vector<int>(chainCount, -1);
	    datfs.seekg(0,ios_base::beg);
	    writeToFile();
   }
   else readFromFile();

   headers = vector <Chain*> (chainCount, NULL);
   int index = 0;
   for(;index < chainCount;index++)
   		headers[index] = new Chain(this, entries.at(index));
}

void ChainHash::writeToFile()
{
	char * content = new char[SINT * 2];
	content = (char*)&chainCount; content += SINT;
	content = (char*)&fb; content -= SINT;
	datfs.write(content, SINT * 2);
	datfs.write((char*)&entries[0], SINT * chainCount);
	delete [] content; content = NULL;
}

void ChainHash::readFromFile()
{
	datfs.seekg(0, ios_base::beg);
	datfs.read((char*)&chainCount, SINT);
	datfs.read((char*)&fb, SINT);
	entries = vector<int>(chainCount, -1);
	datfs.read((char*)&entries[0], SINT * chainCount);
}

bool ChainHash::put(const string&key,const string&value)
{
	int hashVal = hashFunc(key);
	Chain * chain = headers.at(hashVal % chainCount);
	return chain -> put(key, value, hashVal);
}

string ChainHash::get(const string&key)
{
	int hashVal = hashFunc(key);
	Chain * chain = headers.at(hashVal % chainCount);
	return chain -> get(key, hashVal);
}

bool ChainHash::remove(const string&key)
{
	int hashVal = hashFunc(key);
	Chain * chain = headers.at(hashVal % chainCount);
	return chain -> remove(key, hashVal);
}

int defaultHashFunc(const string&str)
{
  int index;
  int value = 0x238F13AF * str.size();
  for(index = 0;index < str.size();index++)
      value = (value + (str.at(index) << (index*5 % 24))) & 0x7FFFFFFF;
  return value;
}