package dev.quassel.ga.core;

public record RunResult<C>(
    int generations,
    EvaluationResult bestEvaluation,
    C bestCandidate,
    String bestDescription
) {
}
