#ifndef CSV_AND_COMPUTATION_HASH_H
#define CSV_AND_COMPUTATION_HASH_H

#include "csv.h"
#include "computation.h"
#include <boost/container_hash/hash.hpp>

std::size_t hash_value(const CSVLine& line) {
	return boost::hash_range(line.values.begin(),line.values.end());
};

std::size_t hash_value(const Computation& computation) {
	static boost::hash<int> hasher;
	auto hash=hasher(computation.primary_input_);
	boost::hash_combine(hash,computation.secondary_inputs_);
	return hash;
};

#endif
