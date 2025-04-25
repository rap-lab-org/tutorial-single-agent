#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <format>
#include "STAstar.hpp"
#include "dynscens.hpp"
#include "gridmap.hpp"
using namespace std;

void save_path(const vector<STAstar::STState>& path, string fn) {
	ofstream fout(fn);
	for (const auto& v: path) {
		fout << v.x << " " << v.y << " " << v.t << endl;
	}
}

void run(movingai::gridmap& g, dynenv::DynScen& scen) {

	STAstar solver(g, scen.node_constraints, g.width_, g.height_);
	auto sy = scen.source / g.width_;
	auto sx = scen.source % g.width_;
	for (auto t: scen.targetSet) {
		auto ty = t / g.width_;
		auto tx = t % g.width_; 
		auto cost = solver.run(sx, sy, tx, ty);
		cout << format("[{}]({}, {}) to [{}]({}, {}): cost {}", 
				scen.source, sx, sy, t, tx, ty, cost) << endl;
		auto path = solver.get_path();
		assert (solver.validate(path));
		save_path(path, to_string(scen.source) + "-" + to_string(t) + "-plan.txt");
	}
}

int main(int argc, char** argv) {
	// ./run_astar <mapfile> <scenfile>
	string mapfile = string(argv[1]);
	string scenfile = string(argv[2]);
	movingai::gridmap g(mapfile);
	dynenv::DynScen scen;
	vector<dynenv::DynScen> scens;
	dynenv::load_and_parse_json(scenfile, scens);
	run(g, scens[0]);
}
