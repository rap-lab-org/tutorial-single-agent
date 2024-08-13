#include <string>
#include <iostream>
#include "gridmap.hpp"

void test_graph_io(std::string filename) {
  movingai::gridmap g(filename);

  std::cout << "map: " << g.filename << std::endl;
  std::cout << "height: " << g.height_ << std::endl;
  std::cout << "width: " << g.width_ << std::endl;
  for (int y=0; y<g.height_; y++) {
    for (int x=0; x<g.width_; x++) 
      std::cout << g.is_obstacle({x, y});
    std::cout << std::endl;
  }
}

int main(int argc, char** argv) {
  //./test_io <mapfile>
  // -> print out the content of map read from file
  std::string filename = std::string(argv[1]);
  test_graph_io(filename);
}
