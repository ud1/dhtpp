#ifndef DHT_KBUCKET_H
#define DHT_KBUCKET_H

#include "types.h"
#include "contact.h"

#include <set>
#include <vector>

namespace dhtpp {

	class CKbucket {
	public:
		enum ErrorCode {
			SUCCEED,
			FULL,
			FAILED,
		};
		
		CKbucket(const BigInt &low_bound, const BigInt &high_bound);

		bool IdInRange(const NodeID &id) const;
		ErrorCode AddContact(const Contact &contact);

		// count = K = number_of_contacts_in_the_holder_bucket
		bool AddContactForceK(const Contact &contact, const NodeID &holder_id, uint16 count);
		bool RemoveContact(const NodeID &id);
		bool GetContact(const NodeID &id, NodeInfo &cont) const;
		bool LastSeenContact(Contact &out) const;
		void GetContacts(std::vector<NodeInfo> &out_contacts) const;
		ErrorCode Split(CKbucket &first, CKbucket &second) const;

		const BigInt &GetHighBound() const {
			return high_bound;
		}

		const BigInt &GetLowBound() const {
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
		BigInt low_bound, high_bound;
	};

}

#endif // DHT_KBUCKET_H
