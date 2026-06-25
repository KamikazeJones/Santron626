package dev.quassel.ga.core;

import java.util.List;
import java.util.Map;

/**
 * Structured evaluation result for a single candidate.
 *
 * <p>Besides the scalar score used by the GA, this record carries metrics and
 * diagnostics that can be queried by tooling, HTTP clients and later analysis
 * modules.
 *
 * @param score scalar optimization target; smaller is better in the current GA
 * @param solved whether the candidate solves the domain problem
 * @param summary human-readable explanation of the result
 * @param metrics structured metric components of the evaluation
 * @param diagnostics textual hints derived from the evaluation
 */
public record EvaluationResult(
    double score,
    boolean solved,
    String summary,
    Map<String, Double> metrics,
    List<String> diagnostics
) {
  /** Creates an immutable evaluation result. */
  public EvaluationResult {
    metrics = Map.copyOf(metrics);
    diagnostics = List.copyOf(diagnostics);
  }
}
