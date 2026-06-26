package dev.quassel.ga.puzzle;

/**
 * Request payload for starting one puzzle run through the local runtime/API.
 *
 * @param size puzzle edge length
 * @param seed random seed for generated boards and GA randomness
 * @param generations generation budget
 * @param population population size
 * @param minLength minimum candidate length
 * @param maxLength maximum candidate length
 * @param boardText optional explicit board override
 */
public record PuzzleRunRequest(
    int size,
    long seed,
    int generations,
    int population,
    int minLength,
    int maxLength,
    String boardText
) {
}
