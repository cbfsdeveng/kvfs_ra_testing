/**
 * Config ... access functions for json format configuration data
 *
 *   Note: if this module is compiled -DTEST it will generate a program
 *	that will process a named file, and then output what it read in
 *	the same format.  If everything is processed correctly (and there
 *	are no gratuitous case/order/whitspace issues) the output should
 *	be identical to the input.
 *
 *	usage: program configfile [directory]
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <json-c/json.h>

#include "Config.h"

// default location for configuration files
#define	CBFS_CFG_DIR	"/etc/cbfs"

// time format: ISO 8601 (GMT)
#define TIMESTAMP	"%Y-%m-%dT%H:%M:%SZ"

/* 
 * a tables to associate multipliers with scalingsuffixes
 *
 *	note that for the reverse mapping to work, the
 *	table must be in ascending numerical order.
 */
struct unit {
	const char *suffix;
	long long multiplier;
};

// units for elapsed time
static struct unit times[] = {
	{ "S", 1 },		/* seconds */
	{ "s", 1 },		/* seconds */
	{ "M", 60 },		/* minutes */
	{ "m", 60 },		/* minutes */
	{ "H", 60*60 },		/* hours */
	{ "h", 60*60 },		/* hours */
	{ "D", 24*60*60 },	/* days */
	{ "d", 24*60*60 },	/* days */
	{  0, 0 }
};

// units for storage capacity
static struct unit sizes[] = {
	{ "B", 1 },
	{ "b", 1 },
	{ "KB", 1024 },
	{ "kb", 1024 },
	{ "MB", 1024 * 1024 },
	{ "mb", 1024 * 1024 },
	{ "GB", 1024LL * 1024LL * 1024LL },
	{ "gb", 1024LL * 1024LL * 1024LL },
	{ "TB", 1024LL * 1024LL * 1024LL * 1024LL },
	{ "tb", 1024LL * 1024LL * 1024LL * 1024LL },
	{ "XB", 1024LL * 1024LL * 1024LL * 1024LL * 1024LL },
	{ "xb", 1024LL * 1024LL * 1024LL * 1024LL * 1024LL },
	{  0, 0 }
};

/*
 * tables to map between input/output strings and the associated enums
 */
struct xlate {
	const char *string;
	int code;
};

// supported key value stores
static struct xlate e_kvs[] = {
	{"LevelDB", (int) Config::LevelDB},
	{"RoxDB", (int) Config::RoxDB},
	{"RAMcloud", (int) Config::RAMcloud},
	{0, 0}
};

// persistence rules
static struct xlate e_persist[] = {
	{"on-write", (int) Config::onWrite},
	{"on-close", (int) Config::onClose},
	{"lazy", (int) Config::lazy},
	{0, 0}
};

// encryption algorithms
static struct xlate e_crypt[] = {
	{"AES-128", (int) Config::AES_128},
	{0, 0}
};

// compression algorithms
static struct xlate e_comp[] = {
	{"ZIP", (int) Config::ZIP},
	{0, 0}
};

// cloud data access protocols
static struct xlate e_protocol[] = {
	{"S3", (int) Config::S3},
	{0, 0}
};

/**
 * constructor - configuration file in default directory
 */
Config::Config( std::string& file ) {
	std::string name = CBFS_CFG_DIR + '/' + file;
	loadFile(name);
}

/**
 * constructor - configuration file in specified directory
 */
Config::Config( std::string& file, std::string& directory) {
	std::string name = directory + '/' + file;
	Config::loadFile(name);
}

/**
 * destructor ... free all allocated memory
 *	particularly important because this is a one-time function
 *	that allocates a lot of buffer/parsing/string space
 */
Config::~Config() {
	if (root != NULL)
		json_object_put(root);
	if (tok != NULL)
		json_tokener_free(tok);
	if (inbuf != NULL)
		free(inbuf);
}

/**
 * loadFile - utility function to open and parse a json file
 *
 * @param	full path to file to be opened
 */
void Config::loadFile( const std::string& path ) {
	root = NULL;
	tok = NULL;
	inbuf = NULL;

	/* open the input file */
	int fd = open(path.c_str(), 0);
	if (fd < 0)
		return;

	/* figure out how long the input file is */
	struct stat statb;
	if (fstat(fd, &statb) < 0)
		return;

	/* read the input file */
	int file_len = statb.st_size;
	inbuf = (char *) malloc(file_len);
	if (inbuf == 0) 
		return;

	int cnt = read(fd, inbuf, file_len);
	if (cnt != file_len)
		return;

	/* create and initialize session structure */
	tok = json_tokener_new();
	root = json_tokener_parse_ex(tok, inbuf, file_len);
}


/**
 * getString - utility function to get the value of a string attribute
 *
 * @param object to search
 * @parm name of desired string attribute
 * @return desired string (or "")
 */
const std::string Config::getString(json_object *obj, const char *name) {

	/* find requested object and make sure it is a string	*/
	json_object *found;
	if (!json_object_object_get_ex(obj, name, &found))
		return std::string();
	if (json_object_get_type(found) != json_type_string)
		return std::string();
	return std::string(json_object_get_string(found));
}

/**
 * getNumber - utility function to return a value of a numeric attribute
 *
 * @param object to search
 * @param name of desired string attribute
 * @param table of multiplier suffixes
 *
 * @return desired number (or 0)
 */
long long Config::getNumber(json_object *obj, const char *name, struct unit *units) {

	/* find requested object and make sure it is a string	*/
	json_object *found;
	if (!json_object_object_get_ex(obj, name, &found))
		return 0;
	if (json_object_get_type(found) == json_type_int)
		return json_object_get_int(found);

	long long val = 0;
	if (json_object_get_type(found) == json_type_string) {
		char *suffix;
		val = strtoll(json_object_get_string(found), &suffix, 10);
		while( units != 0 && units->suffix != 0 ) {
			if (*suffix == units->suffix[0]) {
				val *= units->multiplier;
				break;
			}
			units++;
		}
	}
	return val;
}

/**
 * getEnum - utility function to map a string into an enum
 *
 * @param object to search
 * @param name of desired attribute
 * @param table of strings and enums
 *
 * @return desired value (or 0)
 */
int Config::getEnum(json_object *obj, const char *name, struct xlate *xtable) {
	// find requested object and make sure it is a string
	json_object *found;
	if (!json_object_object_get_ex(obj, name, &found))
		return 0;
	if (json_object_get_type(found) != json_type_string)
		return 0;

	// get the string and match it in the table
	const char * s = json_object_get_string(found);
	for( struct xlate *x = xtable; x->string; x++ ) {
		if (strcmp(s, x->string) == 0)
			return(x->code);
	}
	return 0;
}

//* file system name
const std::string Config::fsName() {
	return getString(root, "name");
}

//* file system UUID
const std::string Config::fsUUID() {
	return getString(root, "uuid");
}

//* current primary
const std::string Config::fsPrimary() {
	json_object *primary;
	if (!json_object_object_get_ex(root, "primary", &primary))
		return std::string();
	return getString(primary, "host");
}

//* current epoch
long Config::fsEpoch() {
	json_object *primary;
	if (!json_object_object_get_ex(root, "primary", &primary))
		return 0;
	return getNumber(primary, "epoch", 0);
}

//* start of current epoch
time_t Config::fsEpochStart() {
	json_object *primary;
	if (!json_object_object_get_ex(root, "primary", &primary))
		return (time_t) 0;
	std::string s = getString(primary, "starting");
	struct tm parsedTime;
	if (strptime(s.c_str(), TIMESTAMP, &parsedTime)) {
		time_t time = mktime(&parsedTime);
		time -= timezone;	// correct to Zulu time
		if (parsedTime.tm_isdst == 1)
			time += 3600;	// correct for daylight savings
		return time;
	} else
		return (time_t) 0;
}

//* location of KVS
const std::string Config::kvsLoc() {
	json_object *kvs;
	if (!json_object_object_get_ex(root, "kvs", &kvs))
		return std::string();
	return getString(kvs, "file");
}

//* type of KVS
enum Config::kvfsImpl Config::kvsType() {
	json_object *kvs;
	if (!json_object_object_get_ex(root, "kvs", &kvs))
		return (enum Config::kvfsImpl) 0;
	return (enum Config::kvfsImpl) getEnum(kvs, "type", e_kvs);
}

//* maximum size of the KVS
long long Config::kvsMaxBytes() {
	json_object *kvs;
	if (!json_object_object_get_ex(root, "kvs", &kvs))
		return 0;
	return getNumber(kvs, "max_size", sizes);
}

//* persistence rules for KVS and cloud updates
enum Config::persistenceRule Config::persistence() {
	json_object *kvs;
	if (!json_object_object_get_ex(root, "kvs", &kvs))
		return (enum Config::persistenceRule) 0;
	return (enum Config::persistenceRule) getEnum(kvs, "persistence", e_persist);
}

//* encryption algorithm (if any)
enum Config::encryptionAlgorithm Config::encryption() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return unencrypted;
	return (enum Config::encryptionAlgorithm) getEnum(tier, "encryption", e_crypt);
}

//* encryption key file
const std::string Config::keyFile() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return std::string();
	return getString(tier, "encryption_key");
}

//* compression algorithm (if any)
enum Config::compressionAlgorithm Config::compression() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return uncompressed;
	return (enum Config::compressionAlgorithm) getEnum(tier, "compression", e_comp);
}

//* location of proxy cache
const std::string Config::cacheDir() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return std::string();
	return getString(tier, "cache_dir");
}

//* target mirroring delay
int Config::mirrorLag() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return 0;
	return getNumber(tier, "mirror_lag", times);
}

//* target free space in kvs
int Config::freePercent() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return 0;
	return getNumber(tier, "free_pct", 0);
}

//* maximum proxy cache size
long long Config::cacheMaxBytes() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return 0;
	return getNumber(tier, "cache_size", sizes);
}

//* maximum cloud object size
long long Config::objectMaxBytes() {
	json_object *tier;
	if (!json_object_object_get_ex(root, "tiering", &tier))
		return 0;
	return getNumber(tier, "max_object", sizes);
}

//* cloud access protocol
enum Config::cloudAccessProtocol Config::cloudProtocol() {
	json_object *cloud;
	if (!json_object_object_get_ex(root, "cloud", &cloud))
		return (enum Config::cloudAccessProtocol) 0;
	return (enum Config::cloudAccessProtocol) getEnum(cloud, "protocol", e_protocol);
}

//* cloud access URL
const std::string Config::cloudEndpoint() {
	json_object *cloud;
	if (!json_object_object_get_ex(root, "cloud", &cloud))
		return std::string();
	return getString(cloud, "endpoint");
}

#ifdef TEST
#include <iostream>
#include <sstream>

// render a date as it would be specified in config file
std::string showDate( time_t when ) {
	struct tm result;
	gmtime_r(&when, &result);

	char timebuf[32];
	strftime(timebuf, sizeof timebuf, TIMESTAMP, &result);
	return std::string(timebuf);
}

// render an integer as it would be specified in config file
std::string showNumber( long long value, struct unit *table ) {
	std::stringstream ss;

	// see if we can find an appropriate divisor
	if (table) {
		struct unit *x = table;
		while (x->suffix && x[1].suffix && value > x[1].multiplier)
			x++;
		ss << value / x->multiplier;
		ss << x->suffix;
	} else
		ss << value;

	return ss.str();
}

// render an enumerated type as it would be specified in config file
std::string showEnum( int code, struct xlate *table ) {
	for( struct xlate *x = table; x->string; x++ ) {
		if (x->code == code)
			return std::string(x->string);
	}
	return std::string("none");
}

/**
 * usage: testpgm cfgfile.json [directory]
 *
 *	read all parameters from specified configuration file
 *		(in default or specified directory)
 *	output the results in the same format
 *		ideally input and output will be identical
 *		(modulo order, case, whitespace)
 */
int main(int argc, char *argv[]) {

	if (argc < 2) {
		exit(-1);
	} 

	std::string file = argv[1];
	std::string dir = (argc > 2) ? argv[2] : "";
	Config *cfg = (argc > 2) ?
		new Config(file, dir) :
		new Config(file);

	// collect all the interesting parameters in printable form
	std::string name = cfg->fsName();
	std::string uuid = cfg->fsUUID();
	std::string primary = cfg->fsPrimary();
	std::string epoch = showNumber(cfg->fsEpoch(), 0);
	std::string since = showDate(cfg->fsEpochStart());
	std::string kvstype = showEnum(cfg->kvsType(), e_kvs);
	std::string kvsloc = cfg->kvsLoc();
	std::string freepct = showNumber(cfg->freePercent(), 0);
	std::string persist = showEnum(cfg->persistence(), e_persist);
	std::string kvssize = showNumber(cfg->kvsMaxBytes(), sizes);
	std::string cachedir = cfg->cacheDir();
	std::string crypt = showEnum(cfg->encryption(), e_crypt);
	std::string key = cfg->keyFile();
	std::string squeeze = showEnum(cfg->compression(), e_comp);
	std::string lag = showNumber(cfg->mirrorLag(), times);
	std::string cachesize = showNumber(cfg->cacheMaxBytes(), sizes);
	std::string objsize = showNumber(cfg->objectMaxBytes(), sizes);
	std::string proto = showEnum(cfg->cloudProtocol(), e_protocol);
	std::string url = cfg->cloudEndpoint();

	// free up the configuation object
	delete cfg;

	// produce output that should be identical to our input
	std::cout << "{\n";
	std::cout << "\t\"name\": \"" + name + "\",\n";
	std::cout << "\t\"uuid\": \"" + uuid + "\",\n";

	std::cout << "\n\t\"primary\": {\n";
	std::cout << "\t    \"host\": \"" + primary + "\",\n";
	std::cout << "\t    \"epoch\": \"" + epoch + "\",\n";
	std::cout << "\t    \"starting\": \"" + since + "\"\n";
	std::cout << "\t},\n";

	std::cout << "\n\t\"kvs\": {\n";
	std::cout << "\t    \"type\": \"" + kvstype + "\",\n";
	std::cout << "\t    \"file\": \"" + kvsloc + "\",\n";
	std::cout << "\t    \"persistence\": \"" + persist + "\",\n";
	std::cout << "\t    \"max_size\": \"" + kvssize + "\"\n";
	std::cout << "\t},\n";

	std::cout << "\n\t\"tiering\": {\n";
	std::cout << "\t    \"cache_dir\": \"" + cachedir + "\",\n";
	std::cout << "\t    \"encryption\": \"" + crypt + "\",\n";
	std::cout << "\t    \"encryption_key\": \"" + key + "\",\n";
	std::cout << "\t    \"compression\": \"" + squeeze + "\",\n";
	std::cout << "\t    \"mirror_lag\": \"" + lag + "\",\n";
	std::cout << "\t    \"cache_size\": \"" + cachesize + "\",\n";
	std::cout << "\t    \"free_pct\": \"" + freepct + "\",\n";
	std::cout << "\t    \"max_object\": \"" + objsize + "\"\n";
	std::cout << "\t},\n";

	std::cout << "\n\t\"cloud\": {\n";;
	std::cout << "\t    \"protocol\": \"" + proto + "\",\n";
	std::cout << "\t    \"endpoint\": \"" + url + "\",\n";
	std::cout << "\t}\n";
	std::cout << "}\n";

}
#endif
