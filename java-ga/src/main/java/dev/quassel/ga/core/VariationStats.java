package dev.quassel.ga.core;

/**
 * Summary of one variation path within a generation.
 *
 * @param attempts number of produced offspring on this path
 * @param improvedCount offspring better than their reference parent score
 * @param neutralCount offspring equal to their reference parent score
 * @param worsenedCount offspring worse than their reference parent score
 * @param meanScoreDelta average of {@code parentScore - childScore}; positive is better
 * @param bestScoreDelta best observed {@code parentScore - childScore}
 */
public record VariationStats(
    int attempts,
    int improvedCount,
    int neutralCount,
    int worsenedCount,
    double meanScoreDelta,
    double bestScoreDelta
) {
}
