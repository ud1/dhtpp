#include "kbucket.h"
#include "timer.h"
#include "config.h"

#include <cassert>
#include <algorithm>

namespace dhtpp {

	CKbucket::CKbucket(const NodeID &low_bound_, const NodeID &high_bound_) {
		low_bound = low_bound_;
		high_bound = high_bound_;
	}

	bool CKbucket::IdInRange(const NodeID &id) const {
		return id >= low_bound && id <= high_bound;
	}

	RoutingTableErrorCode CKbucket::AddContact(const NodeInfo &info) {
		Contact contact;
		(NodeInfo &)contact = info;
		contact.last_seen = GetTimerInstance()->GetCurrentTime();
		return AddContact(contact);
	}

	RoutingTableErrorCode CKbucket::AddContact(const Contact &contact) {
		assert(IdInRange(contact.GetId()));

		if (contacts.size() >= K)
			return FULL;

		std::pair<ContactList::iterator, bool> res = contacts.insert(contact);
		if (res.second)
			return SUCCEED;

		// Update contact
		*res.first = contact;
		return EXISTED;
	}

	RoutingTableErrorCode CKbucket::AddContactForceK(const NodeInfo &info, const NodeID &holder_id, uint16 count) {
		assert(contacts.size() == K);
		if (!count)
			return FULL;

#if !FORCE_K_OPTIMIZATION
		return FULL;
#endif

		// Contacts for sorting
		struct forceK {
			ContactList::iterator it;
			int last_seen_weight, distance_weight;

			const NodeID &GetId() const {
				return it->GetId();
			}

			forceK() {
				last_seen_weight = distance_weight = 0;
			}

			forceK(const forceK &o) {
				*this = o;
			}

			forceK &operator =(const forceK &o) {
				it = o.it;
				last_seen_weight = o.last_seen_weight;
				distance_weight = o.distance_weight;
				return *this;
			}
		} forceK_[K];

		int i;
		ContactList::iterator it;
		for (i = 0, it = contacts.begin(); it != contacts.end(); ++it, ++i)
			forceK_[i].it = it;

		std::partial_sort(forceK_, forceK_ + count, forceK_ + K, distance_comp_ge<forceK>(holder_id));

		// Check this contact in _count_ closest to _holder_id_ contacts
		if ( (forceK_[count - 1].it->id ^ holder_id) < (info.id ^ holder_id) )
			return FULL;

		for (i = 0; i < count; ++i) // only _count_ contacts
			forceK_[i].distance_weight = i;

		// sort by last_seen
		struct last_seen_comp {
			bool operator()(const forceK &f1, const forceK &f2) {
				return f1.it->last_seen < f2.it->last_seen;
			}
		};

		std::sort(forceK_, forceK_ + count, last_seen_comp()); // only _count_ contacts
		for (i = 0; i < count; ++i)
			forceK_[i].last_seen_weight = i;

		// find less useful contact
		int less_useful_ind = 0;
		int less_useful_weight = forceK_[0].last_seen_weight + forceK_[0].distance_weight;
		
		for (i = 1; i < count; ++i) {
			int weight = forceK_[i].last_seen_weight + forceK_[i].distance_weight;
			if (weight < less_useful_weight) {
				less_useful_ind = i;
				weight = less_useful_weight;
			}
		}

		// replace contact
		contacts.erase(forceK_[less_useful_ind].it);
		return AddContact(info);
	}

	bool CKbucket::RemoveContact(const NodeID &id) {
		Contact temp;
		temp.id = id;
		return contacts.erase(temp) > 0;
	}

	bool CKbucket::GetContact(const NodeID &id, Contact &cont) const {
		Contact temp;
		temp.id = id;
		ContactList::const_iterator it = contacts.find(temp);
		if (it == contacts.end())
			return false;
		cont = *it;
		return true;
	}

	bool CKbucket::LastSeenContact(Contact &out) const {
		if (!contacts.size())
			return false;

		ContactList::const_iterator it = contacts.begin(), c = it;
		timestamp stamp = it->last_seen;

		for (; it != contacts.end(); ++it) {
			if (stamp > it->last_seen) {
				stamp = it->last_seen;
				c = it;
			}
		}

		out = *c;

		return true;
	}

	void CKbucket::GetContacts(std::vector<Contact> &out_contacts) const {
		out_contacts.insert(out_contacts.end(), contacts.begin(), contacts.end());
	}

	void CKbucket::GetContacts(std::vector<const Contact *> &out_contacts) const {
		for (ContactList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
			out_contacts.push_back(&*it);
		}
	}

	RoutingTableErrorCode CKbucket::Split(CKbucket &first, CKbucket &second) const {
		ContactList::const_iterator it = contacts.begin(), end_it = contacts.end();
		RoutingTableErrorCode err;
		for (; it != end_it; ++it) {
			if (first.IdInRange(it->GetId())) {
				if ( (err = first.AddContact(*it)) != SUCCEED )
					return err;
			} else {
				assert(second.IdInRange(it->GetId()));
				if ( (err = second.AddContact(*it)) != SUCCEED )
					return err;
			}
		}
		return SUCCEED;
	}

}