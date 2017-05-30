/**
 * Config.h ... access functions for configuration data
 */
#ifndef KVFS_CONFIG
#define KVFS_CONFIG
#include <time.h>
#include <string>

class Config {
	
    public:
	enum kvfsImpl { LevelDB=1, RoxDB, RAMcloud };
	enum persistenceRule { onWrite=1, onClose, lazy };
	enum encryptionAlgorithm { unencrypted=0, AES_128 };
	enum compressionAlgorithm { uncompressed=0, ZIP };
	enum cloudAccessProtocol { local=0, S3 };


	Config( std::string& file );
	Config( std::string& file, std::string& directory);
	~Config();

	// basic file system information
    	const std::string fsName();	// human name
	const std::string fsUUID();	// UUID

	const std::string fsPrimary();	// current primary
	long fsEpoch();			// current epoch #
	time_t fsEpochStart();		// time of epoch start


    	// configuration of the underlying key/value store
	enum kvfsImpl kvsType();	// type of underlying KVS
	const std::string kvsLoc();	// where KVS is stored
	long long kvsMaxBytes();	// maximum size
	enum persistenceRule persistence(); // persistence rules

    	// configuration of the tiering agent
	enum encryptionAlgorithm encryption();	// encryption
	const std::string keyFile();	// location of encryption key
	enum compressionAlgorithm compression(); // compression

	int mirrorLag();		// target mirror delay (seconds)
	int freePercent();		// KVS free space target
	long long objectMaxBytes();	// largest cloud object

	const std::string cacheDir();	// location of proxy cache
	long long cacheMaxBytes();	// maximum proxy cache size

    	// configuration of the cloud proxy
	enum cloudAccessProtocol cloudProtocol();
	const std::string cloudEndpoint(); // cloud access URL

    private:
	char *inbuf;			// input configuration
	struct json_tokener *tok; 	// json-c lexer
	struct json_object *root;	// json-c root object

	// internal utility functions for processing config file
	void loadFile(const  std::string &path );
	const std::string getString(json_object *obj, const char *name);
	long long getNumber(json_object *obj, const char *name, struct unit *units);
	int getEnum(json_object *obj, const char *name, struct xlate *xtable);
};
#endif
