#include "routing_table.h"

#include <cassert>

namespace dhtpp {

	bool CRoutingTable::AddContact(const Contact &contact) {
		BigInt id = (BigInt) contact.node_id;
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		CKbucket::ErrorCode res = it->AddContact(contact);
		if (res == CKbucket::SUCCEED) {
			return true;
		} else if (res == CKbucket::FULL) {
			CKbucketEntry *ptr = &*it;
			if (ptr == holder_bucket) {
				// Split the bucket
				BigInt wid = (ptr->GetHighBound() - ptr->GetLowBound())/2;
				CKbucketEntry *left = new CKbucketEntry(
					ptr->GetLowBound(),
					ptr->GetLowBound() + wid);

				CKbucketEntry *right = new CKbucketEntry(
					ptr->GetLowBound() + wid + 1,
					ptr->GetHighBound());

				ptr->Split(*left, *right);
				buckets.erase(it);
				buckets.insert(*left);
				buckets.insert(*right);
				delete ptr;
				return AddContact(contact);
			} else if (ptr == holder_brother_bucket) {
				// ForceK optimization
				uint16 count = K - holder_bucket->GetContactsNumber();
				assert(count >= 0);
				if (count > 0) {
					return holder_brother_bucket->AddContactForceK(contact, holder_id, count);
				}
			}

			// Do nothing
			return false;
		}

		return false;
	}
}