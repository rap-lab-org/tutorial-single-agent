#pragma once

#include <string>
#include <vector>

namespace movingai {

using namespace std;
using vid = int;

struct State {
  vid x, y;
};

class gridmap {
public:
  // init an empty map
  gridmap(int height, int width);
  // init map based on an input file
  gridmap(const string &filename);

  inline vector<State> get_neighbours(State c) const {
    // TODO: get neighbours of an 8-connected grid;
    auto res = vector<State>{};
    return res;
  };

  // get the label associated with the coordinate (x, y)
  inline bool is_obstacle(State c) const {
    // hit the boundary
    if (c.x < 0 || c.x >= width_ || c.y < 0 || c.y >= height_)
      return true;
    auto label = this->get_label(c);
    return label == 1;
  }

  // set the label associated with the coordinate (x, y)
  inline void set_label(State c, bool label) { db[c.y * width_ + c.x] = label; }

  inline bool get_label(State c) const { return db[c.y * width_ + c.x]; }

  vid height_, width_;
  string filename;
  // whether a location is an obstacle
  vector<bool> db;
};

} // namespace movingai
