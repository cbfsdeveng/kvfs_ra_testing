/*
 * RocksDBFactory.h
 *
 *  Created on: Oct 12, 2015
 *      Author: Prasanna Ponnada
 */

#ifndef ROCKSDBFACTORY_H
#define ROCKSDBFACTORY_H

#include "KVStoreFactory.h"
#include "RocksDBStore.h"

class RocksDBFactory : public KVStoreFactory {
public:
	KVStore* createLocalHDInstance();
	KVStore* createLocalSSDInstance();
	KVStore* createRemoteInstance();
	~RocksDBFactory() {}
};
#endif /* ROCKSDBFACTORY_H */
