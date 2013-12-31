#include "../include/EHash.h"
#include <sys/stat.h>
#include <string.h>

EmptyBlock::EmptyBlock() : curNum(0), nextBlock(-1)
{
}

EmptyBlock::~EmptyBlock()
{
}

bool EmptyBlock::checkSuitable(int size, int & pos)
{
  for(pos = curNum - 1;pos >= 0;pos--)
  {
    if(eles[pos].size > size)
      return true;
  }
  return false;
}

Page::Page(ExtendibleHash * eHash) : d(0),curNum(0)
{
  memset(elements,-1,sizeof(elements));
  this -> eHash = eHash;
}

Page::~Page()
{
}

bool Page::full()
{
  return curNum > PAGESIZE;
}

bool Page::put(const string&key, const string&value, int hashVal)
{
  int index;
  for(index = 0;index < PAGESIZE + 5;index++)
  {
    PageElement element = elements[index];
    if(element.hash_value != -1 && element.hash_value == hashVal)
    {
      if(element.key_size == key.size() && element.data_size == value.size())
      {
        /**
          Read From File, check it whether double
        **/
          eHash -> datfs.seekg(element.data_pointer,ios_base::beg);
          char * content = new char[element.key_size + element.data_size];
          eHash -> datfs.read(content,sizeof(content));
          string str1(content, content + element.key_size);
          delete [] content; content = NULL;
          if(str1 == key) return 0;
      }
    }
  }
  /**
    Find an suitable empty block, if not allocated it at the end of the file
  **/
  int offset = eHash -> findSuitable(key.size() + value.size());
  
  eHash -> datfs.seekg(offset, ios_base::beg);
  eHash -> datfs.write(key.c_str(), sizeof(key));
  eHash -> datfs.write(value.c_str(), sizeof(value));

  return 1;
}

string Page::get(const string&key, int hashVal)
{ 
  int index;
  for(index = 0;index < curNum;index++)
  {
    if(elements[index].hash_value == hashVal && elements[index].key_size == key.size())
    {
      eHash -> datfs.seekg(elements[index].data_pointer + elements[index].key_size, ios_base::beg);
      char * dat =  new char[elements[index].data_size + 1];
      eHash -> datfs.read(dat, elements[index].data_size);
      string value(dat,dat + sizeof(dat) - 1);
      delete [] dat;
      return value;
    }
  }
  return "";
}

bool Page::remove(const string&key, int hashVal)
{
	int index, rindex;
  for(index = 0;index < curNum;index++)
  {
    if(elements[index].hash_value == hashVal && elements[index].key_size == key.size())
    {
      eHash -> datfs.seekg(elements[index].data_pointer, ios_base::beg);
      char * dat =  new char[elements[index].key_size + 1];
      eHash -> datfs.read(dat, elements[index].key_size);
      if(strcmp(dat, key.c_str()) == 0) break;
    }
  }
  if(index == curNum) return false;

  /**
    Attach the space to the emptryBlock
  **/
  eHash -> cycle(elements[index].data_pointer, elements[index].key_size + elements[index].data_size);
  
  for(;index < curNum - 1;index++)
    elements[rindex] = elements[rindex + 1];
  return true;
}

ExtendibleHash::ExtendibleHash(HASH hashFunc)
{
	this -> hashFunc = hashFunc;
	this -> gd = 0; this -> pn = 1;
  this -> fb = -1;
}

ExtendibleHash::~ExtendibleHash()
{
  if(page) delete page;
  page = NULL;
  if(idxfs) idxfs.close();
  if(datfs) datfs.close();
}

bool ExtendibleHash::init(const string&filename)
{
  struct stat buf;
  string idxName = filename + ".idx";
  string datName = filename +  ".dat";

  idxfs.open (idxName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);
  datfs.open (datName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);

  if((stat(idxName.c_str(), &buf) == -1) || buf.st_size == 0)
  { 
    gd = 0; pn = 1;
    entries.push_back(0);
    page = new Page(this);
    
    idxfs.seekg(0,ios_base::beg);
    writeToFile();
    
    datfs.seekg(0,ios_base::beg);
    datfs.write((char*)page,sizeof(Page));
    
    delete page; page = NULL;
  }
  else readFromFile();
}

bool ExtendibleHash::put(const string&key,const string&value)
{
  int hashVal = hashFunc(key);
  int cur     = hashVal & ((1 << gd) -1);
  page        = new Page(this);
  
  datfs.seekg(entries.at(cur), ios_base::beg);
  datfs.read((char*)page, SPAGE);

  if(page -> full() && page -> d == gd)
  {
  		this -> gd++;
  		int oldSize = entries.size();
  		for(int i = 0;i < oldSize;i++)
  		    entries.push_back(entries.at(i));
      
      idxfs.seekg(0,ios_base::end);
      idxfs.write((char*)&entries[oldSize], oldSize*SINT);
  }

	if(page -> full() && page -> d < gd)
	{
      page -> put(key, value, hashVal);
		  
      Page * p1 = new Page(this);
  		  
      int index = 0;int curNum2 = 0, curNum3 = 0;
      
      for(index = 0;index < page -> curNum;index++)
      {
        PageElement element = page -> elements[index];
        datfs.seekg(element.data_pointer,ios_base::cur);
        char * content = new char[element.key_size + 1];
        datfs.read(content, element.key_size);
        string str(content,content+element.key_size);
        delete content; content = NULL;
        int id = hashFunc(str);
        int flag = (id & ((1 << gd) - 1));
        if(((flag >> (page -> d)) & 1) == 1)
          p1 -> elements[curNum3++] = page -> elements[index];
        else
          page -> elements[curNum2++] = page -> elements[index];
      }

      page -> curNum = curNum2; p1 -> curNum = curNum3;

      for(index = 0;index < entries.size();index++)
    	{
        	if(entries.at(index) == entries.at(cur))
        	{
            	if(((index >> (page -> d)) & 1) == 1)
              		entries[index] = datfs.tellg();
        	}
    	}
      page -> d = p1 -> d = (page -> d) + 1;
      datfs.seekg(entries.at(cur),ios_base::beg);
      datfs.write((char*)page, sizeof(Page));

      datfs.seekg(0,ios_base::end);
      datfs.write((char*)p1, sizeof(Page));
	}
  else
   		page -> put(key, value, hashVal);
  delete page;
  page = NULL;
  return true;
}

string ExtendibleHash::get(const string&key)
{
  int hashVal = hashFunc(key);
  int cur     = hashVal & ((1 << gd) -1);
  page        = new Page(this);
  datfs.seekg(entries.at(cur), ios_base::beg);
  datfs.read((char*)page, SPAGE);
  
  string rs = page -> get(key, hashVal);
  
  delete page; page = NULL;
  return rs;
}

bool ExtendibleHash::remove(const string&key)
{
	int hashVal = hashFunc(key);
  int cur     = hashVal & ((1 << gd) -1);
  page        = new Page(this);
  datfs.seekg(entries.at(cur), ios_base::beg);
  datfs.read((char*)page, SPAGE);

  int rb = page -> remove(key, hashVal);
  
  delete page; page = NULL;
  return rb;
}

void ExtendibleHash::writeToFile()
{ 
  char * content = new char[SINT*3];
  content = (char*)&gd; content += 4;
  content = (char*)&pn; content += 4;
  content = (char*)&fb; content -= 8;
  idxfs.write(content,SINT*3);
  idxfs.write((char*)&entries[0], entries.size()*SINT);
  delete [] content; content = NULL;
}

void ExtendibleHash::readFromFile()
{
  idxfs.read((char*)&gd, SINT);
  idxfs.read((char*)&pn, SINT);
  idxfs.read((char*)&fb, SINT);
  entries = vector<int> (1 << gd, 0);
  idxfs.read((char*)&entries[0], entries.size()*SINT);
}

int ExtendibleHash::findSuitable(int size)
{
  EmptyBlock block; int offset, pos; int blockDir;
  if(fb == -1)
  {
    block.curNum = 1;
    block.eles[0].pos = datfs.tellg();
    block.eles[0].size = size;
    offset = datfs.tellg();offset += size;
    datfs.seekg(2*size,ios_base::end);
    datfs.write((char*)&block, sizeof(EmptyBlock));
    return offset;
  }
  
  datfs.seekg(fb, ios_base::cur); 
  datfs.write((char*)&block, sizeof(EmptyBlock));

  blockDir = fb;
  while(block.checkSuitable(size, pos) == false)
  { 
    if(block.nextBlock == -1) break;
    blockDir = block.nextBlock;
    idxfs.seekg(block.nextBlock, ios_base::beg);
    idxfs.read((char*)&block, sizeof(EmptyBlock));
  }
  
  if(block.nextBlock == -1)
  {
    EmptyBlock nBlock;
    nBlock.curNum = 1;
    block.eles[0].pos = datfs.tellg();
    block.eles[0].size = size;
    offset = datfs.tellg(); offset += size;
    datfs.seekg(2 * size, ios_base::end);
    datfs.write((char*)&block, SEBLOCK);
  }
  else
  {
    if(block.eles[pos].size == size)
    {
      block.curNum --;
      offset = block.eles[pos].pos;
      if(block.curNum == 0)
      {
        block.curNum ++;
        block.eles[0].pos = datfs.tellg();
        block.eles[0].size = size;
      }
      datfs.seekg(blockDir, ios_base::beg);
      datfs.write((char*)&block, SEBLOCK);
    }
    else
    {
      offset = block.eles[pos].pos;
      block.eles[pos].size -= size;
      datfs.seekg(blockDir, ios_base::beg);
      datfs.write((char*)&block, SEBLOCK);
    }
  }
  return offset;
}

int defaultHashFunc(const string&str)
{
  int index;
  int value = 0x238F13AF * str.size();
  for(index = 0;index < str.size();index++)
      value = (value + (str.at(index) << (index*5 % 24))) & 0x7FFFFFFF;
  return value;
}

void ExtendibleHash::cycle(int offset, int size)
{
  EmptyBlock block;
  
  if(fb == -1)
  {
    block.curNum = 1;
    block.eles[0].pos = offset;
    block.eles[0].size = size;
    fb = datfs.tellg();
    datfs.seekg(0, ios_base::end);
    datfs.write((char*)&block, sizeof(EmptyBlock));
    return;
  }

  datfs.seekg(fb, ios_base::beg);
  datfs.read((char*)&block, SEBLOCK);
  while(block.curNum == PAGESIZE && block.nextBlock != -1)
  {
    datfs.seekg(block.nextBlock, ios_base::beg);
    datfs.read((char*)&block, SEBLOCK);
  }
  
  if(block.curNum < PAGESIZE)
  {
    block.eles[block.curNum].pos = offset;
    block.eles[block.curNum].size = size;
    block.curNum++;
    datfs.seekg( -SEBLOCK, ios_base::cur);
    datfs.write((char*)&block, SEBLOCK);
  }
  else
  {
    EmptyBlock block2;
    block2.eles[0].pos = offset;
    block2.eles[0].size = size;
    block2.curNum++;
    int curPos = datfs.tellg();
    
    datfs.seekg(0, ios_base::end);
    datfs.write((char*)&block, SEBLOCK);

    block.nextBlock = datfs.tellg();
    block.nextBlock -= SEBLOCK;

    datfs.seekg(curPos - SEBLOCK, ios_base::beg);
    datfs.write((char*)&block, SEBLOCK);
  }
}