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

# Step 4: using space-time `A*` to solve multi-agent pathfinding problem by `Prioritized Planning`

  - [Benchmark](https://movingai.com/benchmarks/mapf/index.html)

# Resources

- [Visualizer](https://github.com/MAPF-Competition/PlanViz)
