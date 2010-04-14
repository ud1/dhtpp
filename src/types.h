#ifndef DHT_TYPES_H
#define DHT_TYPES_H

#include <string>

#include <boost/cstdint.hpp>
#include <boost/mp_math/mp_int.hpp>

namespace dhtpp {
	typedef boost::uint8_t uint8;
	typedef boost::uint16_t uint16;
	typedef boost::uint32_t uint32;
	typedef boost::uint64_t uint64;

	typedef boost::uint64_t timestamp;

	typedef std::string NodeIP;

	typedef boost::mp_math::mp_int<> BigInt;
}

#endif // DHT_TYPES_H
