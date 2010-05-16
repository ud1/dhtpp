#ifndef DHT_NODE_ID_H
#define DHT_NODE_ID_H

#include "config.h"
#include "types.h"

namespace dhtpp {

	// Big endian
	struct NodeID {
		uint8 id[NODE_ID_LENGTH_BYTES];

		bool operator <(const NodeID &o) const;
		bool operator <=(const NodeID &o) const;
		bool operator >=(const NodeID &o) const {
			return !(*this < o);
		}
		bool operator >(const NodeID &o) const {
			return !(*this <= o);
		}
		bool operator ==(const NodeID &o) const;
		NodeID &operator+=(const NodeID &o);
		NodeID &operator+=(unsigned int v);
		NodeID &operator-=(const NodeID &o);
		NodeID &operator >>= (uint16 f);
		NodeID &operator ^= (const NodeID &o);
	};

	inline NodeID operator - (const NodeID &f, const NodeID &s) {
		NodeID res = f;
		return res -= s;
	}

	inline NodeID operator + (const NodeID &f, const NodeID &s) {
		NodeID res = f;
		return res += s;
	}

	inline NodeID operator + (const NodeID &f, unsigned int v) {
		NodeID res = f;
		return res += v;
	}

	inline NodeID operator ^ (const NodeID &f, const NodeID &s) {
		NodeID res = f;
		return res ^= s;
	}

	inline NodeID operator >> (const NodeID &f, uint16 d) {
		NodeID res = f;
		return res >>= d;
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

	inline bool NodeID::operator <=(const dhtpp::NodeID &o) const {
		for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			if (id[i] < o.id[i])
				return true;
			if (id[i] > o.id[i])
				return false;
		}
		return true;
	}

	inline bool NodeID::operator ==(const dhtpp::NodeID &o) const {
		for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			if (id[i] != o.id[i])
				return false;
		}
		return true;
	}

	inline NodeID &NodeID::operator+=(const NodeID &o) {
		uint8 a = 0;
		for (uint16 i = NODE_ID_LENGTH_BYTES; i --> 0; ) {
			uint16 v = id[i] + o.id[i] + a;
			id[i] = v & 0xff;
			a = v >> 8;
		}
		return *this;
	}

	inline NodeID &NodeID::operator+=(unsigned int d) {
		uint8 a = 0;
		for (uint16 i = NODE_ID_LENGTH_BYTES; i --> 0 && (d || a); ) {
			uint16 v = id[i] + (d & 0xff) + a;
			id[i] = v & 0xff;
			a = v >> 8;
			d >>= 8;
		}
		return *this;
	}

	inline NodeID &NodeID::operator-=(const NodeID &o) {
		uint8 a = 0;
		for (uint16 i = NODE_ID_LENGTH_BYTES; i --> 0; ) {
			uint16 s = a + o.id[i];
			if (id[i] < s) {
				id[i] += 256 - s;
				a = 1;
			} else {
				id[i] -= s;
				a = 0;
			}
		}
		return *this;
	}

	inline NodeID &NodeID::operator >>= (uint16 f) {
		uint16 o = f / 8;
		uint16 bts = f % 8;
		uint16 ibts = 8 - bts;
		for (uint16 i = NODE_ID_LENGTH_BYTES; i --> o+1; ) {
			id[i] = (id[i-o] >> bts) | (id[i-o-1] << ibts);
		}
		id[o] = id[0] >> bts;
		return *this;
	}

	inline NodeID &NodeID::operator ^= (const NodeID &o) {
		for (uint16 i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
			id[i] ^= o.id[i];
		}
		return *this;
	}


	struct MaxNodeID : public NodeID {
		MaxNodeID() {
			for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
				id[i] = 0xff;
			}
		}
	};

	struct NullNodeID : public NodeID {
		NullNodeID() {
			for (int i = 0; i < NODE_ID_LENGTH_BYTES; ++i) {
				id[i] = 0x00;
			}
		}
	};
}

#endif // DHT_NODE_ID_H