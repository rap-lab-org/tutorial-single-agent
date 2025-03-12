#include <iostream>
#include <string>
#include "gridmap.hpp"
#include "Astar.hpp"
#include "load_scens.hpp"
using namespace std;

void run(movingai::gridmap& g, movingai::scenario_manager& scenmrg) {

	Astar solver(g, g.width_, g.height_);
	for (int i=0; i<scenmrg.num_experiments(); i++) {
		auto expr = scenmrg.get_experiment(i);
		auto sx = expr->startx();
		auto sy = expr->starty();
		auto gx = expr->goalx();
		auto gy = expr->goaly();

		vector<int> parent;
		auto dist = solver.run(sx, sy, gx, gy, parent);
		printf("From (%d, %d) to (%d, %d) shortest distance: %.2f\n", sx, sy, gx, gy, dist);

		// TODO: postprocess
		// construct the path based on vector<int>parent;
	}
}

int main(int argc, char** argv) {
	// ./run_astar <mapfile> <scenfile>
	string mapfile = string(argv[1]);
	string scenfile = string(argv[2]);
	movingai::gridmap g(mapfile);
	movingai::scenario_manager scenmrg;
	scenmrg.load_scenario(scenfile);
	run(g, scenmrg);
}
