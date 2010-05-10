#ifndef DHT_KBUCKET_H
#define DHT_KBUCKET_H

#include "types.h"
#include "contact.h"
#include "routing_table_error_code.h"

#include <set>
#include <vector>

namespace dhtpp {

	class CKbucket {
	public:
		CKbucket(const NodeID &low_bound, const NodeID &high_bound);

		bool IdInRange(const NodeID &id) const;
		RoutingTableErrorCode AddContact(const NodeInfo &info);
		RoutingTableErrorCode AddContact(const Contact &contact);

		// count = K = number_of_contacts_in_the_holder_bucket
		RoutingTableErrorCode AddContactForceK(const NodeInfo &info, const NodeID &holder_id, uint16 count);
		bool RemoveContact(const NodeID &id);
		bool GetContact(const NodeID &id, Contact &cont) const;
		bool LastSeenContact(Contact &out) const;
		void GetContacts(std::vector<Contact> &out_contacts) const;
		void GetContacts(std::vector<const Contact *> &out_contacts) const;
		RoutingTableErrorCode Split(CKbucket &first, CKbucket &second) const;

		const NodeID &GetHighBound() const {
			return high_bound;
		}

		const NodeID &GetLowBound() const {
			return low_bound;
		}

		uint16 GetContactsNumber() const {
			return (uint16) contacts.size();
		}

	private:
		struct Comp {
			bool operator()(const Contact &c1, const Contact &c2) const {
				return c1.GetId() < c2.GetId();
			}
		};

		typedef std::set<Contact, Comp> ContactList;

		ContactList contacts;
		NodeID low_bound, high_bound;
	};

}

#endif // DHT_KBUCKET_H
