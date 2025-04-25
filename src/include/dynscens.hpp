#pragma once
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <unordered_map>
#include "json.hpp"
#include <vector>

using json = nlohmann::json;

namespace dynenv {
using Time=int;

struct Interval {
	Time tl, tr;
	static const long INF = std::numeric_limits<long>::max();

	bool is_in(Time t) const {
		return tl <= t && t <= tr;
	}
};

using NodeCSTRs = std::unordered_map<long, std::vector<Interval>>;

struct DynScen{
    long source;
    std::vector<long> targetSet;
		NodeCSTRs node_constraints;
    // std::vector<std::vector<long>> node_constraints;
};

inline void load_and_parse_json(const std::string &file_name, std::vector<DynScen> &data_entries) {
    std::ifstream file(file_name);
		json root = json::parse(file);
    auto& data = root["data"];
    for (const auto& entry : data) {
        DynScen data_entry;
        data_entry.source = entry["source"];
        auto& targetSet = entry["targetSet"];
        for (const auto& target : targetSet) {
            data_entry.targetSet.push_back(target);
						// std::cout << target << " ";
        }
				// std::cout << std::endl;

        const auto& nodeConstraints = entry["node_constraints"];
        for (auto it = nodeConstraints.begin(); it != nodeConstraints.end(); ++it) {
            long nodeId = std::stol(it.key()); // 节点ID
            for (auto tupleIt=it->begin(); tupleIt != it->end(); ++tupleIt) {
								long tl = tupleIt->front();
								long tr = tupleIt->back();
								data_entry.node_constraints[nodeId].push_back(Interval{(Time)tl, (Time)tr});
            }
        }
        data_entries.push_back(data_entry);
    }
}

};
