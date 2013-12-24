#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

/*
void walk(const char*fileName)
{
	DIR*dp;
	if((dp = opendir(fileName)) ==NULL)
	{
		printf("error open: %s\n",fileName);
		return 0;
	}
	count=0;
	while ((dirp = readdir(dp)) != NULL)
	{	
		if(dirp->d_name[0]!='.')
		{
			
		}
		if(access(dirp->d_name,X_OK)==0&&dirp->d_name[0]!='.')
		{
			printf("%d: %s\n", count++,dirp->d_name);
			remove(dirp->d_name);
		}	
	}
}
*/

int main(int argc, char *argv[])
{
	DIR*dp;struct dirent*dirp;int count;
	if ((dp = opendir(".")) == NULL)
	{
		printf("error open\n");
		return 0;
	}
	count=0;
	while ((dirp = readdir(dp)) != NULL)
	{	
		if(access(dirp->d_name,X_OK)==0&&dirp->d_name[0]!='.')
		{
			printf("%d: %s\n", count++,dirp->d_name);
			remove(dirp->d_name);
		}	
	}
	closedir(dp);
	return 0;
}