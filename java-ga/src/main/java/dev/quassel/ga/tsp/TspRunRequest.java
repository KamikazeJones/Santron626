package dev.quassel.ga.tsp;

/**
 * Request payload for one travelling-salesman run.
 *
 * @param width map width
 * @param height map height
 * @param cityCount number of cities
 * @param seed random seed for city generation and GA randomness
 * @param generations generation budget
 * @param population population size
 */
public record TspRunRequest(
    int width,
    int height,
    int cityCount,
    long seed,
    int generations,
    int population
) {
}
