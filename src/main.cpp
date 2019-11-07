#include "hashDB.h"
#include "image.h"
#include "transactional.h"
#include "db_init.h"

#include <cstdlib>

void cutestArtists (Database& db)
{
	const auto search = db.SELECT<std::string, double>("artist, (mu - sigma * 3) * min(10, COUNT(*)) as power" 
														" FROM artistScore JOIN image_artist_bridge USING (artist)" 
														" GROUP BY artist ORDER BY power");

	for(const auto& [artist, power] : search)
	{
		std::cout << artist << ' ' << power << std::endl;
	}
}

void cutestCharacters (Database& db)
{
	const auto search = db.SELECT<std::string, double>("character, (mu - sigma * 3) * min(10, COUNT(*)) as power" 
														" FROM characterScore JOIN image_character_bridge USING (character)" );
	for(const auto& [character, power] : search)
	{
		std::cout << character << ' ' << power << std::endl;
	}
}

void readLegacyFiles (Database& db)
{
	auto t = db.transaction();

	insertTags(db, "../booru.csv");
	insertTagScores(db, "../booruScores.csv");
	insertArtists(db, "../artists.csv");
	insertArtistScores(db, "../artistScores.csv");
	insertIDScores(db, "../idScores.csv");
	insertCharacterScores(db, "../charScores.csv");
	insertCharacters(db, "../chars.csv");

	t.commit();;
}

void scanDir (Database& db, const std::string& location)
{
	cute::HashDB hashScan; 

	const auto search = db.SELECT<std::string, double, double, std::string>("* FROM path_meta_data");
	for(const auto& [path, size, time, hash] : search)
	{
		const cute::PathMetaData data (size, time, hash);
		hashScan.readInto(path, data);
	}


	hashScan.scanDirectoryRecursive(location);


	auto transaction = db.INSERT("OR IGNORE INTO path_meta_data "
								 "(path, file_size, write_time, hash) "
								 "VALUES (?, ?, ?, ?)");


	for(const auto& [path, data] : hashScan)
	{
		transaction.push(path.string(), (double) data.file_size, (double) data.write_time, data.hash);
	}
}



int main (int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "usage ./frisbee -i (no args) -d [dbFile] -s [location]" << std::endl;
		std::cerr << "-i	- init by trying to eat legacy cutegrab files from the current dir" << std::endl;
		std::cerr << "-d	- database location" << std::endl;
		std::cerr << "-s	- scan location" << std::endl;

		return 0;
	}

	std::string dbName;
	std::string scanLocation;
	bool init  = false;
	bool dFlag = false;
	bool sFlag = false;

	for(int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if( arg == "-i") 
		{
			init = true;
			continue;
		}

		if( arg == "-d")
		{
			sFlag = false;
			dFlag = true; 
			continue;
		}

		if( arg == "-s")
		{
			dFlag = false; 
			sFlag = true;
			continue;
		}

		if(dFlag)
		{
			dbName = arg;
			continue;
		}

		if(sFlag)
		{
			scanLocation = arg;
			continue;
		}

	}

	if(!dbName.size())
	{
		std::cerr << "no filename provided, using in memory db" << std::endl;
		dbName = ":memory:";
	}

	if(!scanLocation.size())
	{
		std::cerr << "no scan location provided, aborting!" << std::endl;
		return 1; 
	}

	Database db(dbName);
	if(init) readLegacyFiles(db);
	buildDatabases(db);
	scanDir(db, scanLocation);
}
