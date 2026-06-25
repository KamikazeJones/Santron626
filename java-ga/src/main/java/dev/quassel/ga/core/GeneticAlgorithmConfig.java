package dev.quassel.ga.core;

public record GeneticAlgorithmConfig(
    int populationSize,
    int maxGenerations,
    int eliteCount,
    double crossoverRate,
    long seed
) {
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
