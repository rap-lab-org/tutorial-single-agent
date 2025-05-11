#pragma once
#include "dynscens.hpp"
#include "gridmap.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <math.h>
#include <memory>
#include <ostream>
#include <queue>
#include <set>
#include <tuple>
#include <vector>
using namespace std;

class STAstarRC {
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

		friend ostream& operator<<(ostream& o, const STState& s) {
			o << "(" << s.x << ", " << s.y << ", " << s.t << ")";
			return o;
		};
  };

  struct Node {
    STState v;
    Cost g = 0;
    Cost h = 0;
		// Store the self and parent indexes in the container
		ID id = 0;
		ID pa = -1;
		// One may want to use a pointer for the parent, instead of indexes
		//  - Node* pa (raw pointer): cannot guarantee `pa` is not freed 
		//  - shared_ptr<Node> pa: this may cause "cyclic reference", leading to memory leak.
		//  - weak_ptr<Node> pa: should be used
		weak_ptr<Node> pa_ptr;

    Node(int x, int y, Time t = 0, Cost g = 0, Cost h = 0, ID id=0) {
      this->v = STState{x, y, t};
      this->g = g;
      this->h = h;
			this->id = id;
    }

		friend ostream& operator<<(ostream& o, const Node& n) {
			o << "v: " << n.v << " g:" << n.g << " f:" << n.f() << " id:" << n.id;
			return o;
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

  inline shared_ptr<Node> new_node(int x, int y, Time t = 0, Cost g = 0, Cost h = 0) {
    if (nodes.size() + 1 >= nodes.capacity()) {
      nodes.reserve(nodes.capacity() * 2);
    }
		int nid = nodes.size();
    nodes.emplace_back(make_shared<Node>(x, y, t, g, h, nid));
    return nodes.back();
  }

  vector<shared_ptr<Node>> nodes;
  set<tuple<vid, vid, Time>> frontier;
	shared_ptr<Node> bestPtr;
  ID bestID, curID;
  Cost best;

  int width, height;
  const gridmap &grid;
  const NodeCSTRs &cstrs;

  STAstarRC(const gridmap &g, const NodeCSTRs &cs, int w, int h)
      : grid(g), cstrs(cs), width(w), height(h){};

  inline vid id(const vid &x, const vid &y) const { return y * width + x; }

  inline double hVal(const STState &a, const vid &gx, const vid &gy) {
    return abs(a.x - gx) + abs(a.y - gy);
  }

  inline void init_search() {
    this->nodes.clear();
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

  inline bool is_reached(shared_ptr<Node> cptr, int gx, int gy) {
    if (!cptr->isAt(gx, gy)) return false;
		vid cid = id(cptr->v.x, cptr->v.y); 
		if (cstrs.find(cid) == cstrs.end() ||
				cstrs.at(cid).back().tr <= cptr->g) return true;
		return false;
  }

  inline Cost run(int sx, int sy, int gx, int gy) {
    init_search();
		// customize comparator
    auto cmp = [&](const shared_ptr<Node> &i, const shared_ptr<Node> &j) {
			return (*i.get()) < (*j.get());
    };
    priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, decltype(cmp)> q(cmp);
    q.push(new_node(sx, sy));
    best = -1;
    bestID = -1;
    while (!q.empty()) {
      auto cptr = q.top();
      q.pop();
			// cout << "Pop: " << *cptr.get() << endl;
      if (is_reached(cptr, gx, gy)) {
        best = cptr->g;
        bestID = cptr->id;
				bestPtr = cptr;
        break;
      }
      const static int nummoves = 5;
      const static vid dx[] = {0, 0, 1, -1, 0};
      const static vid dy[] = {1, -1, 0, 0, 0};
      const static Cost w[] = {1, 1, 1, 1, 1};
      for (int i = 0; i < nummoves; i++) {
        vid nx = cptr->v.x + dx[i];
        vid ny = cptr->v.y + dy[i];
        Time nt = cptr->v.t + w[i];
        if (grid.is_obstacle({nx, ny}) || !is_safe(nx, ny, nt)) {
          continue;
        }
        if (frontierCheck(nx, ny, nt)) {
          continue;
        }
        auto nptr = new_node(nx, ny, nt);
				nptr->g = cptr->g + w[i];
				nptr->h = hVal(nptr->v, gx, gy);
				nptr->pa = cptr->id;
				nptr->pa_ptr = cptr;
				// cout << "\t Push:" << *nptr.get() << endl;
        q.push(nptr);
        frontier.insert({nx, ny, nt});
      }
    }
    return best;
  }

  inline vector<STState> get_path_fromID() {
    vector<STState> res;
		// by id
    ID cid = bestID;
    while (cid != -1) {
      res.push_back(nodes[cid]->v);
      cid = nodes[cid]->pa;
    }
    reverse(res.begin(), res.end());
    return res;
  }

  inline vector<STState> get_path_fromPtr() {
    vector<STState> res;
		// by shared_ptr
		auto cptr = bestPtr;
    while (cptr) {
      res.push_back(cptr->v);
			cptr = cptr->pa_ptr.lock();
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
        if (l.tl <= v.t && v.t <= l.tr) {
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
