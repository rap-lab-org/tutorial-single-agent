#!/usr/bin/env python
import enum
from collections import defaultdict
from sys import argv
from math import floor
import json
from typing import TypeAlias
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from matplotlib.patches import Rectangle
from matplotlib.collections import Collection, PatchCollection
from matplotlib.animation import FuncAnimation
from matplotlib.typing import ColorType


class Action(str, enum.Enum):
    MOVE = "MOVE"
    WAIT = "WAIT"
    SERVE = "SERVE"
    FINISH = "FINISH"

    def __str__(self):
        return self.value


class Vert:
    x: int
    y: int

    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y

    def __eq__(self, rhs):
        return self.x == rhs.x and self.y == rhs.y

    def __hash__(self):
        return hash((self.x, self.y))

    def __str__(self):
        return f"({self.x}, {self.y})"


State: TypeAlias = tuple[Vert, int]
Trans: TypeAlias = tuple[State, State, Action]

class GridMap:
    height: int
    width: int
    data: list[int]

    def __init__(self, mapfile: str):
        with open(mapfile, "r") as f:
            raws = f.readlines()[1:]
            self.height = int(raws[0].split(' ')[-1])
            self.width = int(raws[1].split(' ')[-1])
            self.data = []
            raws = raws[3:]
            for i in range(self.height*self.width):
                r = self.v2r(i)
                c = self.v2c(i)
                var = 1 if raws[r][c] in list("STW@O") else 0
                self.data.append(var)
            # for r in range(self.height):
            #     for c in range(self.width):
            #         var = 1 if raws[r][c] in list("STW@O") else 0
            #         self.data.append(var)

    def v2r(self, vid: int):
        return vid // self.width

    def v2c(self, vid: int):
        return vid % self.width

    def vid2Vert(self, vid: int) -> Vert:
        x = self.v2c(vid)
        y = self.v2r(vid)
        return Vert(x, y)

    def obstacles(self)->list[Vert]:
        res = []
        for i, v in enumerate(self.data):
            if v == 1:
                res.append(Vert(self.v2c(i), self.v2r(i)))
        return res


def draw_grid(fig: Figure, ax: Axes, bound: tuple[int,int], D: float=1.0) -> tuple[Figure, Axes]:
    maxx, maxy = bound
    xs = [x*D for x in range(maxx+1)]
    ys = [y*D for y in range(maxy+1)]
    ax.vlines(xs, 0, maxy*D, linewidth=0.3, colors='grey')
    ax.hlines(ys, 0, maxx*D, linewidth=0.3, colors='grey')
    return fig, ax


def draw_vert(fig: Figure, ax: Axes, verts: list[Vert], c: str='k', 
              alpha: float=1.0, m: str='o', D: float=1.0, s=10) -> Collection:

    dx, dy = D / 2, D / 2
    xys = [(v.x*D+dx, v.y*D+dy) for v in verts]
    objs: Collection
    if m == "rect":
        objs = PatchCollection([Rectangle((x-dx, y-dy), D, D) for x, y in xys], alpha=alpha, color=c)
    elif m in ["o", "D", "P", "p"]:
        objs = ax.scatter(*zip(*xys), alpha=alpha, marker=m, color=c, s=s)
    elif m == "*":
        objs = ax.scatter(*zip(*xys), alpha=alpha, marker="*", facecolor="none", edgecolors=c, s=s, linewidths=1)
    ax.add_collection(objs)
    return objs


def get_plan(start: Vert, planfile: str) -> list[Trans]:
    res: list[Trans] = []
    if len(planfile) == 0:
        return res
    with open(planfile, "r") as f:
        raws = f.readlines()
    px, py, pt = -1, -1, -1
    for i, line in enumerate(raws):
        if len(line.strip()) == 0:
            continue
        x, y, t = map(int, line.strip('\n').split(' '))
        if i > 0:
            curState = (Vert(x, y), t)
            preState = (Vert(px, py), pt)
            a = "MOVE"
            if x == px and y == py:
                a = "WAIT"
            res.append((preState, curState, Action(a)))
        px, py, pt = x, y, t
    preState = (Vert(px, py), pt)
    curState = (Vert(px, py), pt+1)
    res.append((preState, curState, Action("FINISH")))
    return res


def _draw_ani(g: GridMap, s: Vert, targets: list[Vert], trans: list[Trans], cstrs: dict[Vert, list[int]], 
              dyn_obsts: dict[int, list[State]] | None = None,
              framePerT: int = 2, frame_delay: int = 100, draw_trans: bool = True):
    import matplotlib.colors as mc 
    D = 50
    dx, dy = D / 2, D / 2
    tasks: dict[Vert, Collection] = {}
    cstrObjs: dict[Vert, Collection] = {}
    dynObstObjs: dict[Vert, Collection] = {}
    dynObstID: dict[tuple[Vert, int], int] = {}
    obstColor: dict[int, ColorType] = {}
    servetime: dict[Vert, int] = {}
    if dyn_obsts is None:
        dyn_obsts = defaultdict(list)

    for objid, states in dyn_obsts.items():
        for v, t in states:
            # if (dynObstID.get((v, t)) is not None and dynObstID[(v, t)] != objid):
            #     print (f"({v}, {t})={dynObstID[(v, t)]},  {objid}")
            # assert ( dynObstID.get((v, t)) is None or dynObstID[(v, t)] == objid )
            dynObstID[(v, t)] = objid
        candidate = list(mc.XKCD_COLORS.keys())
        # init colors
        obstColor[objid] = mc.XKCD_COLORS[candidate[objid % len(candidate)]]
    for (v1, t1), (v2, t2), a in trans:
        if a is Action("SERVE"):
            servetime[v2] = t2

    fig, ax = plt.subplots(figsize=(12, 12))
    ax.set_axis_off()
    ax.invert_yaxis()
    fig, ax = draw_grid(fig, ax, (g.width, g.height), D=D)
    draw_vert(fig, ax, g.obstacles(), c='black', alpha=1.0, m='rect', D=D) 
    curPos = draw_vert(fig, ax, [s], c='blue', alpha=0.5, m='o', D=D, s=2*D)
    for t in targets:
        tasks[t] = draw_vert(fig, ax, [t], c='orange', alpha=1, m='*', D=D, s=4*D)
    for v, times in cstrs.items():
        cstrObjs[v] = draw_vert(fig, ax, [v], alpha=0.1, m='rect', D=D, c='gray')
        dynObstObjs[v] = draw_vert(fig, ax, [v], alpha=0.1, m="P", D=D, c='gray')
    for (v1, t1), (v2, t2), a in trans:
        if a is Action("SERVE"):
            servetime[v2] = t2
    initT, finishT = 3, 3
    last, tmax = s, max([max(ts) for ts in cstrs.values()])
    if len(trans) > 0:
        (_,_),(last,tmax),_ = trans[-1]
    totalT = tmax + initT + finishT
    numframes = totalT * framePerT
    header = ax.annotate(
        f"[START] ({s.x}, {s.y}) at 0",
        xy=(g.width*D*0.75, g.height*D*0.1),
        bbox={'facecolor': 'black', 'alpha': 0.5, 'pad': 2},
        ha="center", va="center",
        color="snow",
    )

    def update_plot(frame: int):
        tl = frame // framePerT
        tr = tl
        if frame % framePerT > 0:
            tr += 1
        tl %= totalT
        # start
        if tl < initT:
            pass
        # executing
        elif tl >= initT and tl < initT + tmax:
            if len(trans) > tl-initT:
                (v1,t1), (v2,t2), action = trans[tl-initT]
                if action is Action("MOVE"):
                    header.set_text(f"[MOVE] ({v1.x}, {v1.y}) at {t1}")
                elif action is Action("SERVE"):
                    header.set_text(f"[SERVE] ({v1.x}, {v1.y}) at {t1}")
                elif action is Action("WAIT"):
                    header.set_text(f"[WAIT] ({v1.x}, {v1.y}) at {t1}")
                perc = (frame % framePerT) / framePerT
                if not draw_trans:
                    perc = floor(perc)
                x = v1.x + (v2.x - v1.x) * perc
                y = v1.y + (v2.y - v1.y) * perc
                curPos.set_offsets((x*D+dx, y*D+dy))
            else:
                header.set_text(f"[Env] at {tl}")

            for v, obj in cstrObjs.items():
                obst = dynObstObjs.get(v)
                assert(obst is not None)
                if (tl-initT) in cstrs[v]:
                    oid = dynObstID.get((v, tl-initT))
                    if oid is not None:
                    # if oid is not None:
                    # print (f"{v} {tl-initT}, color: {obstColor[oid]}")
                        obst.set_color(obstColor[oid])
                        obst.set_alpha(0.5)
                    obj.set_alpha(0.5)
                else:
                    obst.set_alpha(0.1)
                    obst.set_color("gray")
                    obj.set_alpha(0.1)
            for v, task in tasks.items():
                if servetime.get(v, tmax + initT) <= (tl-initT):
                    task.set_facecolor("orange")
                else:
                    task.set_facecolor("none")
        # finish
        else:
            for v, task in tasks.items():
                task.set_facecolor("orange")
            header.set_text(f"[FINISH] ({last.x}, {last.y}) at {tmax}")
        return ([header] + list(tasks.values()) + list(cstrObjs.values()) + [curPos])

    # Create animation
    from tqdm.auto import tqdm
    import numpy as np

    ani = FuncAnimation(
        fig,
        update_plot,
        frames=tqdm(np.arange(numframes), initial=1), # type: ignore
        interval=frame_delay,
        blit=True,
    )

    return ani


def parse_dyn_obst(dynobstfile):
    res: dict[int, list[State]] = defaultdict(list)
    if len(dynobstfile) > 0:
        data = json.load(open(dynobstfile, 'r'))
        for v, states in data.items():
            for state in states:
                x, y, t = map(int, state)
                res[int(v)].append((Vert(x, y), t))
    return res


def draw_animation(mapfile: str, injson: str, planfile: str, dynobstfile: str = "", framePerT: int = 2, frame_delay: int = 100, 
                   draw_trans: bool = True):
    g = GridMap(mapfile)
    inputData = json.load(open(injson,'r'))['data'][0]
    s: Vert = g.vid2Vert(inputData['source'])
    targets: list[Vert] = [g.vid2Vert(i) for i in inputData['targetSet']]
    ncs = inputData['node_constraints']
    cstrs: dict[Vert, list[int]] = defaultdict(list)
    dyn_obsts: dict[int, list[State]] = parse_dyn_obst(dynobstfile)
    for vid, times in ncs.items():
        v = g.vid2Vert(int(vid))
        for tl, tr in times:
            for t in range(tl, tr+1):
                cstrs[v].append(t)
    trans = get_plan(s, planfile)
    ani = _draw_ani(g, s, targets, trans, cstrs, dyn_obsts)
    return ani


if __name__ == "__main__":
    """
    ./viz.py <mapfile> <jsonfile> <planfile>
    """
    mapfile = argv[1]  # static obstacles
    jsonfile = argv[2] # start, tasks, dynamic obstacles
    planfile = argv[3] # tour
    save = 1
    if len(argv) > 4:
        save = int(argv[4])
    ani = draw_animation(mapfile, jsonfile, planfile, "", framePerT=5, draw_trans=False)
    if save:
        ani.save("sipp-tsp.mp4")
    else:
        plt.show()
