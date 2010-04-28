#ifndef DHT_CONTACT_H
#define DHT_CONTACT_H

#include "types.h"
#include "node_id.h"

namespace dhtpp {

	struct NodeAddress {
		NodeIP ip;
		uint16 port;

		NodeAddress &operator =(const NodeAddress &o) {
			ip = o.ip;
			port = o.port;
			return *this;
		}

		bool operator != (const NodeAddress &o) {
			return (ip != o.ip) || (port != o.port);
		}
	};

	struct NodeInfo : public NodeAddress {
		NodeID id;

		const NodeID &GetId() const {
			return id;
		}

		NodeInfo &operator =(const NodeInfo &o) {
			*(NodeAddress *) this = o;
			id = o.id;
			return *this;
		}
	};

	struct Contact : public NodeInfo {
		timestamp last_seen;

		Contact() {}
		Contact(const Contact &o) {
			*this = o;
		}

		Contact &operator =(const Contact &o) {
			*(NodeInfo *) this = o;
			last_seen = o.last_seen;
			return *this;
		}
	};

	template<typename T>
	struct distance_comp_ge {
		distance_comp_ge(const BigInt &holder_id_) : holder_id(holder_id_) {};

		bool operator()(const T &f1, const T &f2) {
			return ((BigInt)f1.GetId() ^ holder_id) >= // Greater or equal
				((BigInt)f2.GetId() ^ holder_id);
		}

		BigInt holder_id;
	};

	template<typename T>
	struct distance_comp_le {
		distance_comp_le(const BigInt &holder_id_) : holder_id(holder_id_) {};

		bool operator()(const T &f1, const T &f2) {
			return ((BigInt)f1.GetId() ^ holder_id) <= // Less or equal
				((BigInt)f2.GetId() ^ holder_id);
		}

		BigInt holder_id;
	};

}

#endif // DHT_CONTACT_H
