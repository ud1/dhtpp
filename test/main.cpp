#include "../src/kbucket.h"

#include <cassert>

using namespace dhtpp;

void testKBucket() {
	BigInt high_bound;
	high_bound.pow2(8*NODE_ID_LENGTH_BYTES);
	high_bound--;
	CKbucket bucket(0, high_bound);

	// bucket is empty
	Contact temp;
	assert(bucket.LastSeenContact(temp) == false);
	assert(bucket.RemoveContact(temp.id) == false);

	// insert one contact
	temp.last_seen = 10;
	memset(temp.id.id, 0, sizeof(temp.id.id));
	assert(bucket.AddContact(temp) == CKbucket::SUCCEED);
	Contact temp2;
	assert(bucket.LastSeenContact(temp2) == true);
	assert(temp.id == temp2.id);
	assert(bucket.RemoveContact(temp2.id) == true);
	assert(bucket.RemoveContact(temp2.id) == false);

	// insert several contacts
	for (int i = 0; i < K-1; ++i) {
		temp.id.id[0] = i;
		temp.last_seen = i + 10;
		assert(bucket.AddContact(temp) == CKbucket::SUCCEED);
	}

	// Already exist check
	for (int i = 0; i < K-1; ++i) {
		temp.id.id[0] = i;
		temp.last_seen = 0;
		assert(bucket.AddContact(temp) == CKbucket::FAILED);
	}

	temp.id.id[0] = K-1;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == CKbucket::SUCCEED);

	// Check FULL
	temp.id.id[0] = K;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == CKbucket::FULL);

	temp.id.id[0] = 5;
	assert(bucket.RemoveContact(temp.id) == true);
}

int main() {
	testKBucket();

	return 0;
}