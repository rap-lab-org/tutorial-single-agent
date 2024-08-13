#include <iostream>
#include <string>
#include "load_scens.hpp"

void test_scen_io(std::string filename) {
  movingai::scenario_manager scenmgr;
  scenmgr.load_scenario(filename);

  for (int i=0; i<scenmgr.num_experiments(); i++) {
    auto expr = scenmgr.get_experiment(i);
    std::cout << expr->mapheight() << " " << expr->mapwidth() << " "
              << expr->startx() << " " << expr->starty() << " "
              << expr->goalx() << " " << expr->goaly() << " "
              << expr->distance() << std::endl;
        
  }
}

int main(int argc, char** argv) {
  std::string filename = std::string(argv[1]);
  test_scen_io(filename);
}
