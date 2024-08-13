#pragma once
//
// A parser for gridmap files written in Nathan Sturtevant's HOG format.
//
// @author: dharabor
// @created: 08/08/2012
//

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

namespace movingai {

bool traversable(char c);

class gm_header {
public:
  gm_header(unsigned int height, unsigned int width, const char *type)
      : height_(height), width_(width), type_(type) {}

  gm_header() : height_(0), width_(0), type_("") {}

  gm_header(const movingai::gm_header &other) { (*this) = other; }

  virtual ~gm_header() {}

  gm_header &operator=(const movingai::gm_header &other) {
    this->height_ = other.height_;
    this->width_ = other.width_;
    this->type_ = other.type_;
    return *this;
  }

  unsigned int height_;
  unsigned int width_;
  std::string type_;
};

class gm_parser {
public:
  gm_parser(const std::string &filename);
  ~gm_parser();

  inline movingai::gm_header get_header() { return this->header_; }

  inline uint32_t get_num_tiles() { return this->map_.size(); }

  inline char get_tile_at(unsigned int index) {
    if (index >= this->map_.size()) {
      std::cerr << "err; gm_parser::get_tile_at "
                   "invalid index "
                << index << std::endl;
      exit(1);
    }
    return this->map_.at(index);
  }

private:
  gm_parser(const gm_parser &) {}
  gm_parser &operator=(const gm_parser &) { return *this; }

  void parse_header(std::fstream &);
  void parse_map(std::fstream &);

  std::vector<char> map_;
  gm_header header_;
};

class experiment {
public:
  experiment(unsigned int sx, unsigned int sy, unsigned int gx, unsigned int gy,
             unsigned int mapwidth, unsigned int mapheight, double d,
             std::string m)
      : startx_(sx), starty_(sy), goalx_(gx), goaly_(gy), mapwidth_(mapwidth),
        mapheight_(mapheight), distance_(d), map_(m), precision_(4) {}
  ~experiment() {}

  inline unsigned int startx() { return startx_; }

  inline unsigned int starty() { return starty_; }

  inline unsigned int goalx() { return goalx_; }

  inline unsigned int goaly() { return goaly_; }

  inline double distance() { return distance_; }

  inline std::string map() { return map_; }

  inline unsigned int mapwidth() { return mapwidth_; }

  inline unsigned int mapheight() { return mapheight_; }

  inline int precision() { return precision_; }

  inline void set_precision(unsigned int prec) { precision_ = prec; }

  inline void set_precision(int prec) { precision_ = prec; }

private:
  unsigned int startx_, starty_, goalx_, goaly_;
  unsigned int mapwidth_, mapheight_;
  double distance_;
  std::string map_;
  unsigned int precision_;

  // no copy
  experiment(const experiment &other) {}
  experiment &operator=(const experiment &other) { return *this; }
};

class scenario_manager {
public:
  std::string mapfile;
  scenario_manager();
  ~scenario_manager();

  inline experiment *get_experiment(unsigned int which) {
    if (which < experiments_.size()) {
      return experiments_[which];
    }
    return 0;
  }

  inline void add_experiment(experiment *newexp) {
    experiments_.push_back(newexp);
  }

  inline unsigned int num_experiments() { return experiments_.size(); }

  inline size_t mem() {
    return sizeof(*this) + sizeof(experiment) * experiments_.size();
  }

  void load_scenario(const std::string &filelocation);

  std::string last_file_loaded() { return sfile_; }

private:
  void load_v1_scenario(std::ifstream &infile);

  std::vector<experiment *> experiments_;
  int version_;
  std::string sfile_;
};
}; // namespace movingai
