package dev.quassel.ga.core;

/**
 * Final result of a completed or stopped run.
 *
 * @param <C> candidate type
 * @param runId unique ID of the run
 * @param generations last processed generation index
 * @param bestEvaluation best structured evaluation observed in the run
 * @param bestCandidate best candidate observed in the run
 * @param bestDescription human-readable description of the best candidate
 * @param run runtime object that contains snapshots and lifecycle state
 */
public record RunResult<C>(
    String runId,
    int generations,
    EvaluationResult bestEvaluation,
    C bestCandidate,
    String bestDescription,
    SearchRun<C> run
) {
}
