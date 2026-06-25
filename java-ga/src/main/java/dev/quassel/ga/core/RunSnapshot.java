package dev.quassel.ga.core;

/**
 * Public snapshot of a run at a given point in time.
 *
 * @param <C> candidate type
 * @param runId unique ID of the run
 * @param status lifecycle state at snapshot time
 * @param generationStats statistics of the recorded generation
 */
public record RunSnapshot<C>(
    String runId,
    RunStatus status,
    GenerationStats<C> generationStats
) {
}
