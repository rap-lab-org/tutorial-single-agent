# Step 1: getting start with classic single agent planner

0. Implement `get_neighbours` in `src/source/gridmap.cpp`;

    - You may need to consider `no-corner-cutting` in step 2.

1. Implement `Dijkstra`;

2. Implement `A*`;

# Step 2: running benchmark

- [MovingAI - Dao](https://movingai.com/benchmarks/dao/index.html)
- [MovingAI - others](https://movingai.com/benchmarks/grids.html)

**Note**: [corner-cutting](https://github.com/gppc-dev/startkit-classic/blob/master/Problem_Definition.md) is not
allowed in all these benchmark instances. Specifically:

> For a diagonal move (dx, dy) from the position (x, y), corner-cutting occurs if either (x+dx, y) or (x, y+dy) is not
> traversable

Disallowing `corner-cutting` may lead to different optimal path.



# Step 3: implementing space-time `A*` to solve single-agent with dynamic obstacles

## Problem Statement

- Graph: 4-connected grid map, i.e., agent only moves in four directions `left, right, up, down`, each move takes only 1
  time step
- Constraints: the location of the agent `p` at any time step `t` must be obstacle-free, i.e.:
    - `p` cannot be a static obstacle, which is described in the `*.map` file
    - `t` cannot fall within any constrained interval at `p`, which is described in the `*.json` file, for example:
        ```json
            "node_constriants": {
                "780": [
                    [0, 1], [82, 88]
                ],
                ...
            }
        ```
        A node with id `780` is occupied by dynamic obstacles during the time steps `[0, 1]` and `[82, 88]` (inclusive).

- Objective: find the minimum **time cost** path from `start` to `goal`

## Subtasks

1. Filling up blank in `STAStar.hpp` to implement a space-time A\*, and test via `test/run_stastar.cpp`.
    ```bash
    ./build/run_stastar ../maps/random-32-32-10.map ../scens/random-100-10.json

    ```
    This will run multiple `start/goal` instances, and save each path to a file (e.g., `835-253-plan.txt`) 
2. Use the visualizer in `python/` to draw the animation, i.e.,
    ```bash
    # navigate to `python/`, then run the following command
    ./viz.py ../maps/random-32-32-10.map ../scens/100-10-10.json ../src/835-172-plan.txt

    ```
3. Generalize your `STAstar` solver to 8-connected grid map, where the time cost of a diagonal move becomes $\sqrt{2}$.

# Step 4: using space-time `A*` to solve multi-agent pathfinding problem by `Prioritized Planning`

  - [Benchmark](https://movingai.com/benchmarks/mapf/index.html)

# Resources

- [Visualizer](https://github.com/MAPF-Competition/PlanViz)
