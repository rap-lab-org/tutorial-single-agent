#pragma once
#include "dynscens.hpp"
#include "gridmap.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <ostream>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace std;

#define SIPPDBG 0

class SIPP {
public:
  using gridmap = movingai::gridmap;
  using UnsafeInv = dynenv::Interval;
  using Time = dynenv::Time;
  using Cost = int;
  using Coord = int; // coordinate
  using ID = int; // search node id
  using VID = long;
  using EID = long long; // edge id
  using NodeCSTRs = unordered_map<VID, vector<UnsafeInv>>;
  using EdgeCSTRs = unordered_map<EID, vector<UnsafeInv>>;
  const Time INFT = numeric_limits<Time>::max() / 2;

  struct STState {
    VID x, y;
    Time t;

    friend ostream &operator<<(ostream &o, const STState &s) {
      o << "(" << s.x << ", " << s.y << ", " << s.t << ")";
      return o;
    };
  };

  // For lazy initialization:
  // if `GVar.__round` is not same as the current round,
  // then regard `g` as an uninitialized value
  struct GVar {
    Cost g;
    int __round;
  };

  struct SafeInterval {
    Time tl, tr;
    int key;

    friend ostream &operator<<(ostream &o, const SafeInterval &n) {
      o << "#" << n.key << "-[" << n.tl << "," << n.tr << ")";
      return o;
    }

    bool is_overlapped(Time l, Time r) { return !(tl >= r || tr <= l); }
  };

  struct Node {
    STState v;
    SafeInterval si;
    Cost g = 0;
    Cost h = 0;
    // Store the self and parent indexes in the container
    ID id = 0;
    ID pa = -1;
    // One may want to use a pointer for the parent, instead of indexes
    //  - Node* pa (raw pointer): cannot guarantee `pa` is not freed
    //  - shared_ptr<Node> pa: this may cause "cyclic reference", leading to
    //  memory leak.
    //  - weak_ptr<Node> pa: should be used
    weak_ptr<Node> pa_ptr;

    Node(int x, int y, Time t, SafeInterval si, Cost g = 0, Cost h = 0,
         ID id = 0)
        : si(si), g(g), h(h), id(id) {
      this->v = STState{x, y, t};
      this->g = g;
      this->h = h;
      this->id = id;
    }

    friend ostream &operator<<(ostream &o, const Node &n) {
      o << "v: " << n.v << " g:" << n.g << " f:" << n.f() << " id:" << n.id
        << " " << n.si;
      return o;
    }

    inline bool isAt(VID x, VID y) const { return v.x == x && v.y == y; }

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

  inline shared_ptr<Node> new_node(int x, int y, Time t, SafeInterval si,
                                   Cost g = 0, Cost h = 0) {
    if (nodes.size() + 1 >= nodes.capacity()) {
      nodes.reserve(nodes.capacity() * 2);
    }
    int nid = nodes.size();
    nodes.emplace_back(make_shared<Node>(x, y, t, std::move(si), g, h, nid));
    return nodes.back();
  }

  vector<shared_ptr<Node>> nodes;
  vector<vector<GVar>> gtable;
  shared_ptr<Node> bestPtr;
  ID bestID, curID;
  Cost best;
  int __round = 0; // lazy initialization

  int width, height;
  const gridmap &graph;
  const NodeCSTRs &cstrs_n;
  const EdgeCSTRs &cstrs_e;

  SIPP(const gridmap &g, const NodeCSTRs &cs, const EdgeCSTRs &ecs, int w,
       int h)
      : graph(g), cstrs_n(cs), cstrs_e(ecs), width(w), height(h) {
    gtable.resize(w * h);
    for (int i = 0; i < h * w; i++) {
      if (cstrs_n.find(i) == cstrs_n.end())
        gtable[i].resize(1);
      else
        gtable[i].resize(cstrs_n.at(i).size() + 1);
    }
  };

  inline VID id(const VID &x, const VID &y) const { return y * width + x; }

  inline Cost gval(VID cid, int key) {
    if (gtable[cid][key].__round == __round)
      return gtable[cid][key].g;
    else
      return INFT;
  }

  inline double hVal(const STState &a, const VID &gx, const VID &gy) {
    return abs(a.x - gx) + abs(a.y - gy);
  }

  inline void init_search() {
    this->nodes.clear();
    // cstrs might be different each round
    // ensure gtable[i] has enough size
    for (int i = 0; i < height * width; i++) {
      if (cstrs_n.find(i) != cstrs_n.end() &&
          gtable[i].size() < cstrs_n.at(i).size() + 1) {
        gtable[i].resize(cstrs_n.at(i).size() + 1);
      }
    }
    __round++;
  }

  inline int last_unsafe_before_tl(int lb, int ub, Time tl,
                                   const vector<UnsafeInv> &unsafes) {
    // find the last i that unsafes[i].tl<=tl
    int res = -1;
    while (lb < ub) {
      int mid = (lb + ub) >> 1;
      if (unsafes.at(mid).tl <= tl) {
        res = lb;
        lb = mid + 1;
      } else
        ub = mid;
    }
    return res;
  }

  inline vector<SafeInterval>
  _find_safe_intvs(Time tl, Time tr, const vector<UnsafeInv> &unsafes) {

    vector<SafeInterval> res = {};
    res.reserve(1 << 5);
    // the first unsafe interval that unsafes[i].tl <= tl
    int i = last_unsafe_before_tl(0, unsafes.size(), tl, unsafes);
    Time safeL, safeR;
    if (i == -1) {
      // all unsafes start after tl, there is a safe interval [0, unsafes[i].tl)
      res.push_back({0, unsafes.front().tl, 0});
      // then start the scanning from the first unsafe
      i = 0;
    }
    // invariant
    // [safeL, safeR) is a safeinterval between unsafes[i] and unsafes[i+1]
    // the id of the safe interval is `i+1`
    safeL = unsafes[i].tr;
    while (i + 1 < unsafes.size() && safeL < tr) {
      safeR = unsafes.at(i + 1).tl;
      res.push_back({safeL, safeR, i + 1});
      // next safeL
      safeL = unsafes.at(i + 1).tr;
      ++i;
    }
    // postprocess
    if (safeL < tr) {
      safeR = INFT;
      res.push_back({safeL, safeR, i + 1});
      assert(i + 1 == unsafes.size());
    }
    return res;
  }

  // find all safe intervals that are potentially overlapped
  // with a time interval [tl, tr)
  inline vector<SafeInterval> find_safe_intvs(VID cur_id, VID nxt_id, Time tl,
                                              Time tr, Cost w) {
    vector<SafeInterval> node_safes;
    if (cstrs_n.find(nxt_id) == cstrs_n.end()) {
      // nid is always safe
      node_safes = vector<SafeInterval>{{0, INFT, 0}};
    } else {
      node_safes = _find_safe_intvs(tl + w, tr + w, cstrs_n.at(nxt_id));
    }
    return node_safes;
    // TODO: add edge constraints
    vector<SafeInterval> edge_safes;
    auto ekey = cur_id * 1000000 + nxt_id;
    if (cstrs_e.find(ekey) == cstrs_e.end()) {
      // edge is always safe
      edge_safes = vector<SafeInterval>{{0, INFT, 0}};
    } else {
      edge_safes = _find_safe_intvs(tl, tr, cstrs_e.at(ekey));
    }
    // filter node_safes by edges_safes
    // auto res = func(node_safes, edges_safes)
    // return res;
  }

  inline bool is_reached(shared_ptr<Node> cptr, int gx, int gy) {
    if (!cptr->isAt(gx, gy))
      return false;
    int cstrs_num = 0;
    VID cid = id(cptr->v.x, cptr->v.y);
    if (cstrs_n.find(cid) != cstrs_n.end())
      cstrs_num = cstrs_n.at(cid).size();
    return cptr->si.key == cstrs_num;
  }

  inline Cost run(int sx, int sy, int gx, int gy) {
    init_search();
    // customize comparator
    auto cmp = [&](const shared_ptr<Node> &i, const shared_ptr<Node> &j) {
      return (*i.get()) < (*j.get());
    };
    priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, decltype(cmp)> q(
        cmp);
    // ensure the start location is safe, otherwise no solution
    auto safe_intvs = find_safe_intvs(-1, id(sx, sy), 0, 1, 0);
    for (auto &safe_intv : safe_intvs) {
      if (safe_intv.tl <= 0 && 0 < safe_intv.tr) {
        gtable[id(sx, sy)][safe_intv.key] = {0, __round};
        q.push(new_node(sx, sy, 0, safe_intv));
        break;
      }
    }
    best = -1;
    bestID = -1;
    while (!q.empty()) {
      auto cptr = q.top();
      q.pop();
			if (SIPPDBG) cout << "Pop: " << *cptr.get() << endl;
      if (is_reached(cptr, gx, gy)) {
        best = cptr->g;
        bestID = cptr->id;
        bestPtr = cptr;
        break;
      }
      VID cid = id(cptr->v.x, cptr->v.y);
      assert(cptr->si.key < gtable[cid].size());
      if (gval(cid, cptr->si.key) < cptr->g)
        continue;
      const static int nummoves = 4;
      const static VID dx[] = {0, 0, 1, -1, 0};
      const static VID dy[] = {1, -1, 0, 0, 0};
      const static Cost w[] = {1, 1, 1, 1, 1};
      for (int i = 0; i < nummoves; i++) {
        Coord nx = cptr->v.x + dx[i];
        Coord ny = cptr->v.y + dy[i];
        if (graph.is_obstacle({nx, ny})) {
          continue;
        }
        // current can reach the suc in interval [tl, tr)
        Time tl = cptr->v.t, tr = cptr->si.tr;
        auto nid = id(nx, ny);
        auto save_intvs = find_safe_intvs(cid, nid, tl, tr, w[i]);
        // iterate all safe intervals that overlapped with [tl, tr)
				tl += w[i], tr += w[i];
        for (auto &safe_intv : save_intvs)
          if (safe_intv.is_overlapped(tl, tr)) {
            Time nt = max(safe_intv.tl, tl);
            VID nid = id(nx, ny);
            // already explored
            if (gval(nid, safe_intv.key) <= nt)
              continue;
            gtable[nid][safe_intv.key] = {nt, __round};
            // the earlies time step to enter position (nx, ny),
            // within the safe interval `intv`
            auto nptr = new_node(nx, ny, nt,
                                 {safe_intv.tl, safe_intv.tr, safe_intv.key});
            nptr->g = nt;
            nptr->h = hVal(nptr->v, gx, gy);
            nptr->pa = cptr->id;
            nptr->pa_ptr = cptr;
            if (SIPPDBG) cout << "\t Push:" << *nptr.get() << endl;
            q.push(nptr);
          }
      }
    }
    return best;
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
      VID key = id(v.x, v.y);
      if (cstrs_n.find(key) == cstrs_n.end())
        continue;
      for (const auto &l : cstrs_n.at(key)) {
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
