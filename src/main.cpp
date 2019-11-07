#include "hashDB.h"
#include "image.h"
#include "transactional.h"
#include "db_init.h"
#include "utility.h"

#include <cstdlib>
#include <algorithm>

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

void twitter (Database& db)
{

	const auto cutest = db.SELECT<std::string, std::string,  double> ("path, hash, (mu - sigma * 3) as score FROM "
								  "path_meta_data JOIN idScore USING (hash) "
								  "WHERE hash NOT IN (SELECT hash FROM used) "
								  "ORDER BY score DESC LIMIT 500");


	const auto random = select_randomly(cutest.begin(), cutest.end());
	if(random == cutest.end()) return;
	const auto [path, hash, score] = *random; 

	std::string qHash = '"' + hash;
	qHash = qHash + '"';

	const auto characters = db.SELECT<std::string>("character FROM image_character_bridge WHERE hash = "+ qHash);
	const auto artists	  = db.SELECT<std::string>("artist FROM image_artist_bridge WHERE hash = " + qHash);
	const auto cutestTags = db.SELECT<std::string, double>("tag, (mu - sigma * 3) * min(10, COUNT(*)) "
														  "as power FROM tagScore JOIN image_tag_bridge USING (tag) GROUP BY tag");
	auto tags		  = db.SELECT<std::string>("tag FROM image_tag_bridge WHERE hash = " + qHash);



	std::sort(tags.begin(), tags.end(), 
	[&](const auto a, const auto b)
	{
		const auto [aTag] = a; 
		const auto [bTag] = b;
		double aPower = 0;
		double bPower = 0;

		for(const auto& [tag, power] : cutestTags)
		{
			if(tag == aTag) aPower = power;
			if(tag == bTag) bPower = power;
		}

		return aPower > bPower;
	});

	std::cout << '"' << path << '"' << std::endl;
	std::cout << "cutescore: " << score << std::endl;
	if(artists.size())
	{
		std::cout << "artist: ";
		for(const auto& [a] : artists) std::cout << a << ' ';
	}
	if(characters.size()) std::cout << std::endl;
	if(characters.size())
	{
		std::cout << "character: ";
		for(const auto& [c] : characters) std::cout << '#' << c << ' ';
	}
	int i = 0;
	std::cout << std::endl;
	std::cout << std::endl;
	for(const auto& [tag] : tags) 
	{
		if(tag == "no_sauce") continue;
		if(tag == "no_gelbooru") continue;
		if(tag == "no_danbooru") continue;
		if(tag == "danbooru") continue;

		std::cout << '#' << tag << std::endl;
		if(i++ > 3) break;
	}

//	auto u = db.INSERT("OR IGNORE INTO used (hash) VALUES (?)");
//	u.push(hash);
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
	bool scan;
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
			scan = true;
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

	if(!scanLocation.size() && scan)
	{
		std::cerr << "no scan location provided, aborting!" << std::endl;
		return 1; 
	}

	Database db(dbName);
	buildDatabases(db);

	if(init) readLegacyFiles(db);
	auto t = db.transaction();
	if(scan) scanDir(db, scanLocation);
	t.commit();
	if(scan) twitter(db);

}
