#ifndef _ENV_H
#define _ENV_H

/*#include "../include/customDB.h"
*/
#include <stdio.h>

class Env1{
public:
	Env1(/*CustomDB * db*/);
    ~ Env1();

public:
	bool init();

private:
	/*CustomDB * db;*/
	FILE	 * idxFile;
	FILE     * datFile;
};

#endif