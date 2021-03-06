#include "hashDB.h" 
#include "fileMd5.h"
#include "image.h"
#include <iomanip>
#include <cassert>
#include <iostream>
#include <set>

#include <sys/types.h>
#include <sys/stat.h>

namespace cute {


HashDB :: HashDB ()
{
}

HashDB :: ~HashDB (void)
{
}
void HashDB :: readInto (const std::filesystem::path p, const PathMetaData d)
{
	pathMap.insert( std::make_pair( p, d));
}


void HashDB :: scanDirectory (std::filesystem::path p)
{

	for(const auto& f : std::filesystem::directory_iterator(p))
	{

		if(!conformingFileType(f.path())) continue;

		/*
		const auto rawTime = std::filesystem::last_write_time(f);
		const auto epoch = rawTime.time_since_epoch();
		const auto value = std::chrono::duration_cast<std::chrono::seconds>(epoch);

		const unsigned long time = value.count();
		*/
		struct stat attrib; 
		stat(f.path().string().c_str(), &attrib);
		const unsigned long time = attrib.st_mtime; 

		const unsigned long size = std::filesystem::file_size(f);
		const auto absolute = std::filesystem::absolute(f.path());

		const auto it = pathMap.find(absolute);
		
		//if our file does contain our path
		if(it != pathMap.end())
		{
			//if the store metadata is still accurate, we add it to our localPathMap
			if(it->second.file_size == size && it->second.write_time == time)
			{
				localPathMap.insert_or_assign(absolute, it->second);

				std::cout << "using stored metadata for " << f.path().string() << '\n'; 
				continue;
			}
		}
		std::cout << "stored metadata for " << f.path().string() << " was no longer valid\n"; 

		//if the stored metadata is no longer accurate we replace the store metadata and rehash the file
		const Hash hash = hashFile(absolute);

		pathMap.insert_or_assign( absolute,  PathMetaData(size, time, hash));
		localPathMap.insert_or_assign(absolute, PathMetaData(size, time, hash));
	}

}

void HashDB :: scanDirectoryRecursive (std::filesystem::path p)
{

	for(const auto& f : std::filesystem::recursive_directory_iterator(p, std::filesystem::directory_options::follow_directory_symlink))
	{


		if(!conformingFileType(f.path())) continue;
		/*
		const auto rawTime = std::filesystem::last_write_time(f);
		const auto epoch = rawTime.time_since_epoch();
		const auto value = std::chrono::duration_cast<std::chrono::seconds>(epoch);

		const unsigned long time = value.count();
		*/
		struct stat attrib; 
		stat(f.path().string().c_str(), &attrib);
		const unsigned long time = attrib.st_mtime; 
	
		const unsigned long size = std::filesystem::file_size(f);
		const auto absolute = std::filesystem::absolute(f.path());

		const auto it = pathMap.find(absolute);
		
		//if our file does contain our path
		if(it != pathMap.end())
		{

			//if the store metadata is still accurate, we add it to our localPathMap
			if(it->second.file_size == size && it->second.write_time == time)
			{
				localPathMap.insert_or_assign(absolute, it->second);

				continue;
			}
		}

		//if the stored metadata is no longer accurate we replace the store metadata and rehash the file
		const Hash hash = hashFile(absolute);

		pathMap.insert_or_assign( absolute,  PathMetaData(size, time, hash));
		localPathMap.insert_or_assign(absolute, PathMetaData(size, time, hash));
	}


}

bool HashDB :: contains (const std::filesystem::path p) const
{
	const auto absolute = std::filesystem::absolute(p);

	return localPathMap.find(absolute) != localPathMap.end();
}

PathMetaData HashDB :: retrieveData (const std::filesystem::path p) const
{
	const auto absolute = std::filesystem::absolute(p);

	assert (localPathMap.find(absolute) != localPathMap.end());
	return localPathMap.find(absolute)->second;
}

std::map<std::filesystem::path, PathMetaData>::const_iterator HashDB :: begin (void) const
{
	return localPathMap.begin();
}


std::map<std::filesystem::path, PathMetaData>::const_iterator HashDB :: end   (void) const
{
	return localPathMap.end();
}


}
