#include "gridmap.hpp"
#include "load_scens.hpp"

using namespace movingai;

gridmap::gridmap(vid h, vid w) {
  this->db.resize(h * w);
  fill(this->db.begin(), this->db.end(), false);
};

gridmap::gridmap(const string &filename) {
  gm_parser parser(filename);
  this->height_ = parser.get_header().height_;
  this->width_ = parser.get_header().width_;
  this->db.resize(this->height_ * this->width_);
  for (vid i = 0; i < this->db.size(); i++) {
    auto c = parser.get_tile_at(i);
    this->db[i] = traversable(c) ? 0 : 1;
  }
}

vector<State> get_neighbours(State c) {
  // TODO: get neighbours of an 8-connected grid;
  auto res = vector<State>{};
  return res;
}
