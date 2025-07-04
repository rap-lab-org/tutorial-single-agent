#include "SIPP.hpp"
#include "STAstar.hpp"
#include "STAstarRC.hpp"
#include "dynscens.hpp"
#include "gridmap.hpp"
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

void save_path(const vector<STAstar::STState> &path, string fn) {
  ofstream fout(fn);
  for (const auto &v : path) {
    fout << v.x << " " << v.y << " " << v.t << endl;
  }
}

void run(movingai::gridmap &g, dynenv::DynScen &scen) {

	SIPP::EdgeCSTRs ecstrs = {};
  STAstar solver(g, scen.node_constraints, g.width_, g.height_);
  SIPP sipp(g, scen.node_constraints, ecstrs, g.width_, g.height_);
  // STAstarRC solverRC(g, scen.node_constraints, g.width_, g.height_);
  auto sy = scen.source / g.width_;
  auto sx = scen.source % g.width_;
  for (auto t : scen.targetSet) {
    auto ty = t / g.width_;
    auto tx = t % g.width_;
    cout << format("[{}]({}, {}) to [{}]({}, {})", scen.source, sx, sy, t, tx, ty) << endl; 

    auto tstart = std::chrono::steady_clock::now();
    auto cost = solver.run(sx, sy, tx, ty);
    auto tnow = std::chrono::steady_clock::now();
    double tcost = chrono::duration<double>(tnow - tstart).count();
		cout << format("\tSTAstar: cost {} runtime: {:3f}s", cost, tcost) << endl;
    auto path = solver.get_path();
    assert(solver.validate(path));
    save_path(path, to_string(scen.source) + "-" + to_string(t) + "-plan.txt");

    tstart = std::chrono::steady_clock::now();
    auto costSIPP = sipp.run(sx, sy, tx, ty);
    tnow = std::chrono::steady_clock::now();
    tcost = chrono::duration<double>(tnow - tstart).count();
    cout << format("\tSIPP: cost {} runtime: {:3f}s", costSIPP, tcost) << endl;
    auto path_sipp = sipp.get_path_fromPtr();
    assert(sipp.validate(path_sipp));
    // auto pathRC = solverRC.get_path_fromID();
    // assert (solverRC.validate(pathRC));
    // pathRC = solverRC.get_path_fromPtr();
    // assert (solverRC.validate(pathRC));
    assert(costSIPP == cost);
  }
}

int main(int argc, char **argv) {
  // ./run_astar <mapfile> <scenfile>
  string mapfile = string(argv[1]);
  string scenfile = string(argv[2]);
  movingai::gridmap g(mapfile);
  dynenv::DynScen scen;
  vector<dynenv::DynScen> scens;
  dynenv::load_and_parse_json(scenfile, scens);
  run(g, scens[0]);
}
