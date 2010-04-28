#ifndef DHT_CONFIG_H
#define DHT_CONFIG_H

#include "types.h"

namespace dhtpp {

	const uint16 NODE_ID_LENGTH_BYTES = 20;
	const uint16 K = 16;
	const uint16 alpha = 3;
	const uint64 timeout_period = 500; // ms
	const uint16 attempts_number = 2;

}

#endif // DHT_CONFIG_H
