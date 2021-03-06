#ifndef DHT_CONFIG_H
#define DHT_CONFIG_H

#include "types.h"

namespace dhtpp {

	const uint16 NODE_ID_LENGTH_BYTES = 20;
	const uint16 K = 10;
	const uint16 alpha = 3;
	const uint64 timeout_period = 500; // ms
	const uint16 attempts_number = 2;
	const uint64 republish_time = 3600*1000;
	const uint64 republish_time_delta = 60*1000;
	const uint64 expiration_time = 24*3600*1000;
	const uint64 begin_stats = 40*60*1000;
	const double avg_on_time		= 20*60*1000;
	const double avg_on_time_delta = 120*1000;
	const double avg_off_time	= 20*60*1000;
	const double avg_off_time_delta = 120*1000;
	const uint64 check_node_time_interval = 1000;
	const uint64 check_value_time_interval = 5000;
	const int values_per_node = 10;
	const uint64 min_rt_check_time_interval = 60*1000;
	const int republish_treshhold = 4;

	const uint64 network_delay = 50;
	const uint64 network_delay_delta = 50;
	const float packet_loss = 0.1f;

	const uint64 run_time = avg_on_time + avg_off_time + 5*60*1000;

	const uint16 rt_b = 2;
	const uint16 rt_r = 1; // rt_r <= rt_b

	const uint16 rt_pow2_r = 1 << rt_r;
	const uint16 rt_pow2_b_r = 1 << (rt_b - rt_r);
	const uint16 hld_br_buck_count = (rt_pow2_r-1)*rt_pow2_b_r;

#define FORCE_K_OPTIMIZATION 1
#define DOWNLIST_OPTIMIZATION 1
}

#endif // DHT_CONFIG_H
