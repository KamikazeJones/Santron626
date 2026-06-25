package dev.quassel.ga.core;

public record GenerationStats<C>(
    int generation,
    double bestScore,
    double averageScore,
    boolean solved,
    C bestCandidate,
    String bestDescription
) {
}
