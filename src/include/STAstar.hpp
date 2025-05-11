#pragma once
#include "dynscens.hpp"
#include "gridmap.hpp"
#include <algorithm>
#include <cassert>
#include <math.h>
#include <queue>
#include <set>
#include <tuple>
#include <vector>
using namespace std;

class STAstar {
public:
  using gridmap = movingai::gridmap;
  using Time = dynenv::Time;
  using vid = movingai::vid;
  using Cost = int;
  using ID = int;
  using NodeCSTRs = std::unordered_map<long, std::vector<dynenv::Interval>>;

  struct STState {
    vid x, y;
    Time t;
  };

  struct Node {
    STState v;
    Cost g = 0;
    Cost h = 0;

    Node(int x, int y, Time t = 0, Cost g = 0, Cost h = 0) {
      this->v = STState{x, y, t};
      this->g = g;
      this->h = h;
    }

    inline bool isAt(vid x, vid y) const { return v.x == x && v.y == y; }

    inline double f() const { return g + h; }

    // Define priority: which one should be expand earlier than others
    // what if modify the priority?
    inline bool operator<(const Node &rhs) const {
      if (f() == rhs.f())
        return g > rhs.g;
      else
        return f() > rhs.f();
    }
  };

  inline ID gen_node(int x, int y, Time t = 0, Cost g = 0, Cost h = 0) {
    if (nodes.size() + 1 >= nodes.capacity()) {
      nodes.reserve(nodes.capacity() * 2);
    }
    nodes.emplace_back(x, y, t, g, h);
    parent.push_back(-1);
    return nodes.size() - 1;
  }
  vector<Node> nodes;
  vector<int> parent;
  set<tuple<vid, vid, Time>> frontier;
  ID bestID, curID;
  Cost best;

  int width, height;
  const gridmap &grid;
  const NodeCSTRs &cstrs;

  STAstar(const gridmap &g, const NodeCSTRs &cs, int w, int h)
      : grid(g), cstrs(cs), width(w), height(h){};

  inline vid id(const vid &x, const vid &y) const { return y * width + x; }

  inline double hVal(const STState &a, const vid &gx, const vid &gy) {
    return abs(a.x - gx) + abs(a.y - gy);
  }

  inline void init_search() {
    this->nodes.clear();
    this->parent.clear();
    this->frontier.clear();
  }

  inline bool is_safe(const vid &x, const vid &y, const vid &t) {
    auto nid = id(x, y);
    auto invlsEntry = cstrs.find(nid);
    // no constraints at (x, y)
    if (invlsEntry == cstrs.end())
      return true;
    const auto &invls = invlsEntry->second;
    for (const auto &invl : invls) {
      if (invl.is_in(t))
        return false;
    }
    return true;
  }

  bool frontierCheck(vid x, vid y, Time t) {
    return frontier.find({x, y, t}) != frontier.end();
  }

  inline const Node &cur() const { return this->nodes.at(curID); }

  inline bool is_reached(int gx, int gy) {
    auto &n = cur();
    if (!n.isAt(gx, gy)) return false;
		vid cid = id(n.v.x, n.v.y); 
		if (cstrs.find(cid) == cstrs.end() ||
				cstrs.at(cid).back().tr <= n.g) return true;
		return false;
  }

  inline Cost run(int sx, int sy, int gx, int gy) {
    init_search();
    auto pcmp = [&](const ID &i, const ID &j) {
      return this->nodes[i] < this->nodes[j];
    };
    priority_queue<int, vector<int>, decltype(pcmp)> q(pcmp);
    q.push(gen_node(sx, sy));
    best = -1;
    bestID = -1;
    while (!q.empty()) {
      curID = q.top();
      q.pop();
      if (is_reached(gx, gy)) {
        best = cur().g;
        bestID = curID;
        break;
      }

      const static int nummoves = 5;
      const static vid dx[] = {0, 0, 1, -1, 0};
      const static vid dy[] = {1, -1, 0, 0, 0};
      const static Cost w[] = {1, 1, 1, 1, 1};
      for (int i = 0; i < nummoves; i++) {
        vid nx = cur().v.x + dx[i];
        vid ny = cur().v.y + dy[i];
        Time nt = cur().v.t + w[i];
        if (grid.is_obstacle({nx, ny}) || !is_safe(nx, ny, nt)) {
          continue;
        }
        if (frontierCheck(nx, ny, nt)) {
          continue;
        }
        ID nid = gen_node(nx, ny, nt);
        nodes[nid].g = cur().g + w[i];
        nodes[nid].h = hVal(nodes[nid].v, gx, gy);
        parent[nid] = curID;
        q.push(nid);
        frontier.insert({nx, ny, nt});
      }
    }
    return best;
  }

  inline vector<STState> get_path() {
    vector<STState> res;
    ID cid = bestID;
    while (cid != -1) {
      res.push_back(nodes[cid].v);
      cid = parent[cid];
    }
    reverse(res.begin(), res.end());
    return res;
  }

  inline bool validate(const vector<STState> &path) {
    for (const auto &v : path) {
      vid key = id(v.x, v.y);
      if (cstrs.find(key) == cstrs.end())
        continue;
      for (const auto &l : cstrs.at(key)) {
        if (l.tl <= v.t && v.t < l.tr) {
          cerr << std::format("Violate constraint at loc ({}, {}), time {}",
                              v.x, v.y, v.t)
               << endl;
          return false;
        }
      }
    }
    return true;
  }
};
