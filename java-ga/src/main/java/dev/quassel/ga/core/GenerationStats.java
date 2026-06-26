package dev.quassel.ga.core;

import java.util.Map;

/**
 * Snapshot-style statistics for a single generation of a run.
 *
 * @param <C> candidate type
 * @param generation zero-based generation index
 * @param bestScore best score in the current population
 * @param bestScoreImprovement improvement over the previous generation's best score
 * @param averageScore arithmetic mean score of the population
 * @param scoreStdDev standard deviation of the score distribution
 * @param uniqueCandidateRatio ratio of unique candidate fingerprints in the population
 * @param stagnationGenerations generations since the last best-score improvement
 * @param solvedCount number of solved candidates in the population
 * @param solved whether the best candidate is solved
 * @param averageMetrics population-wide mean values of domain metrics
 * @param mutationOnlyStats effect summary for mutation-only offspring
 * @param crossoverMutationStats effect summary for crossover-plus-mutation offspring
 * @param bestCandidate best candidate of the generation
 * @param bestDescription human-readable description of the best candidate
 * @param bestEvaluation structured evaluation result of the best candidate
 */
public record GenerationStats<C>(
    int generation,
    double bestScore,
    double bestScoreImprovement,
    double averageScore,
    double scoreStdDev,
    double uniqueCandidateRatio,
    int stagnationGenerations,
    int solvedCount,
    boolean solved,
    Map<String, Double> averageMetrics,
    VariationStats mutationOnlyStats,
    VariationStats crossoverMutationStats,
    C bestCandidate,
    String bestDescription,
    EvaluationResult bestEvaluation
) {
  /** Creates an immutable statistics snapshot. */
  public GenerationStats {
    averageMetrics = Map.copyOf(averageMetrics);
  }
}
