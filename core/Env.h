#ifndef _ENV_H
#define _ENV_H

class Env{
public:
	Env(CustomDB * db);
  ~ Env(CustomDB * db);

public:
	bool init();

private:
	CustomDB * db;
	FILE	 * idxFile;
	FILE     * datFile;
};

#endif