#ifndef DHT_CONTACT_H
#define DHT_CONTACT_H

#include "types.h"
#include "node_id.h"

namespace dhtpp {

	struct Contact {
		NodeID node_id;
		NodeIP node_ip;
		uint16 node_port;
		timestamp last_seen;

		Contact() {}
		Contact(const Contact &o);
		Contact &operator =(const Contact &o);

		const NodeID &GetId() const {
			return node_id;
		}
	};

	inline Contact::Contact(const Contact &o) {
		*this = o;
	}

	inline Contact &Contact::operator =(const Contact &o) {
		node_id = o.node_id;
		node_ip = o.node_ip;
		node_port = o.node_port;
		last_seen = o.last_seen;
		return *this;
	}
}

#endif // DHT_CONTACT_H
