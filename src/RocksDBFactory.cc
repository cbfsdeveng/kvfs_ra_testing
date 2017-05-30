/*
 * RocksDBFactory.cc
 *
 *  Created on: Oct 12, 2015
 *      Author: Prasanna Ponnada
 */
#include <iostream>
#include "KVStoreFactory.h"
#include "RocksDBFactory.h"
#include "RocksDBStore.h"

KVStore*
RocksDBFactory::createLocalHDInstance() {
	return new RocksDBStore;
}

KVStore*
RocksDBFactory::createLocalSSDInstance() {
	std::cout << "RocksDB on an SSD is not supported at this time." << "\n";
	return nullptr;
}

KVStore*
RocksDBFactory::createRemoteInstance() {
	std::cout << "RocksDB on a remote node is not supported at this time." <<
			"\n";
	return nullptr;
}
