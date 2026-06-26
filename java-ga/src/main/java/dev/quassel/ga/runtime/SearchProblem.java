package dev.quassel.ga.runtime;

import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.SearchDomain;
import java.util.Map;

/**
 * Domain-facing contract used by the generic runtime layer.
 *
 * @param <C> candidate type
 * @param <R> request/specification type
 */
public interface SearchProblem<C, R> {
  /**
   * Creates the domain instance for one run request.
   *
   * @param request typed run request
   * @return domain instance for the run
   */
  SearchDomain<C> createDomain(R request);

  /**
   * Creates the static GA configuration for one run request.
   *
   * @param request typed run request
   * @return GA configuration
   */
  GeneticAlgorithmConfig createConfig(R request);

  /**
   * Returns normalized request fields for external inspection.
   *
   * @param request typed run request
   * @return stable string map of effective request parameters
   */
  Map<String, String> describeRequest(R request);
}
