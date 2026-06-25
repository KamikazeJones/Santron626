package dev.quassel.ga.core;

/**
 * Static configuration for a GA run.
 *
 * @param populationSize number of candidates per generation
 * @param maxGenerations hard generation limit
 * @param eliteCount number of elite candidates copied unchanged
 * @param crossoverRate probability of crossover before mutation
 * @param seed random seed for reproducible runs
 */
public record GeneticAlgorithmConfig(
    int populationSize,
    int maxGenerations,
    int eliteCount,
    double crossoverRate,
    long seed
) {
  /** Validates the configuration invariants. */
  public GeneticAlgorithmConfig {
    if (populationSize < 2) {
      throw new IllegalArgumentException("populationSize must be >= 2");
    }
    if (maxGenerations < 1) {
      throw new IllegalArgumentException("maxGenerations must be >= 1");
    }
    if (eliteCount < 0 || eliteCount >= populationSize) {
      throw new IllegalArgumentException("eliteCount must be >= 0 and < populationSize");
    }
    if (crossoverRate < 0.0 || crossoverRate > 1.0) {
      throw new IllegalArgumentException("crossoverRate must be between 0 and 1");
    }
  }
}
