#ifndef DHT_NODE_ID_H
#define DHT_NODE_ID_H

#include "config.h"
#include "types.h"

namespace dhtpp {

	struct NodeID {
		uint8 id[NODE_ID_LENGTH_BYTES];
		operator BigInt() const;
		bool operator <(const NodeID &o) const;
		bool operator ==(const NodeID &o) const;
	};

	inline NodeID::operator BigInt() const {
		const char hex_digit[] = "0123456789abcdef";
		char hex[2*NODE_ID_LENGTH_BYTES + 3];

		hex[0] = '0';
		hex[1] = 'x';
		for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			hex[2 + 2*i + 0] = hex_digit[id[i] / 16];
			hex[2 + 2*i + 1] = hex_digit[id[i] % 16];
		}
		hex[2*NODE_ID_LENGTH_BYTES + 2] = '\0';

		return BigInt(hex);
	}

	inline bool NodeID::operator <(const dhtpp::NodeID &o) const {
		for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			if (id[i] < o.id[i])
				return true;
			if (id[i] > o.id[i])
				return false;
		}
		return false;
	}

	inline bool NodeID::operator ==(const dhtpp::NodeID &o) const {
		for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			if (id[i] != o.id[i])
				return false;
		}
		return true;
	}

	struct MaxNodeID : public NodeID {
		MaxNodeID() {
			for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
				id[i] = 255;
			}
		}
	};
}

#endif // DHT_NODE_ID_H