
#ifndef HASH_DB 
#define HASH_DB 

#include <filesystem> 
#include <map>
#include <fstream>
#include <string>
#include "image.h"
#include "filetypes.h"


namespace cute {



class HashDB
{

	using Hash = PathMetaData::Hash;

	std::map<std::filesystem::path, PathMetaData> pathMap;
	std::map<std::filesystem::path, PathMetaData> localPathMap;


public:
	HashDB ();
	~HashDB (void);

	void scanDirectory (std::filesystem::path p);
	void scanDirectoryRecursive (std::filesystem::path p);

	void readInto (const std::filesystem::path p, const PathMetaData d);

	bool contains (const std::filesystem::path p) const;
	PathMetaData retrieveData (const std::filesystem::path p) const;

	std::map<std::filesystem::path, PathMetaData>::const_iterator  begin (void) const;
	std::map<std::filesystem::path, PathMetaData>::const_iterator  end   (void) const;

};
}
#endif


