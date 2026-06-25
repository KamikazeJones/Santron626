package dev.quassel.ga.core;

/**
 * Snapshot-style statistics for a single generation of a run.
 *
 * @param <C> candidate type
 * @param generation zero-based generation index
 * @param bestScore best score in the current population
 * @param averageScore arithmetic mean score of the population
 * @param scoreStdDev standard deviation of the score distribution
 * @param uniqueCandidateRatio ratio of unique candidate fingerprints in the population
 * @param solvedCount number of solved candidates in the population
 * @param solved whether the best candidate is solved
 * @param bestCandidate best candidate of the generation
 * @param bestDescription human-readable description of the best candidate
 * @param bestEvaluation structured evaluation result of the best candidate
 */
public record GenerationStats<C>(
    int generation,
    double bestScore,
    double averageScore,
    double scoreStdDev,
    double uniqueCandidateRatio,
    int solvedCount,
    boolean solved,
    C bestCandidate,
    String bestDescription,
    EvaluationResult bestEvaluation
) {
}
