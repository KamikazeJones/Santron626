package dev.quassel.ga.core;

public record EvaluationResult(
    double score,
    boolean solved,
    String summary
) {
}
