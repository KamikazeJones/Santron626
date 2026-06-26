package dev.quassel.ga.runtime;

/**
 * Installable problem adapter for the generic runtime and CLI.
 *
 * @param <C> candidate type
 * @param <R> request/specification type
 */
public interface RegisteredProblem<C, R> extends SearchProblem<C, R>, StartRequestParser<R> {
  /**
   * Stable problem identifier used by CLI, workers and registry routing.
   *
   * @return unique problem ID
   */
  String problemId();

  /**
   * Runs problem-specific smoke and invariants.
   */
  void selfTest();
}
