package dev.quassel.ga.runtime;

import dev.quassel.ga.core.GeneticAlgorithm;
import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.RunRegistry;
import dev.quassel.ga.core.RunMetadata;
import dev.quassel.ga.core.SearchDomain;
import dev.quassel.ga.core.SearchRun;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Generic runtime service that executes typed run requests.
 *
 * @param <C> candidate type
 * @param <R> request/specification type
 */
public final class SearchRuntimeService<C, R> implements AutoCloseable {
  private final SearchProblem<C, R> problem;
  private final RunRegistry<C> registry = new RunRegistry<>();

  /**
   * Creates a runtime service for one problem adapter.
   *
   * @param problem domain adapter
   */
  public SearchRuntimeService(SearchProblem<C, R> problem) {
    this.problem = problem;
  }

  /**
   * Starts one asynchronous run.
   *
   * @param request typed run request
   * @return runtime object for the new run
   */
  public SearchRun<C> start(R request) {
    GeneticAlgorithmConfig config = problem.createConfig(request);
    RunMetadata metadata = new RunMetadata(problem.describeRequest(request), describeConfig(config));
    SearchRun<C> run = registry.start((runId, runtime) -> {
      SearchDomain<C> domain = problem.createDomain(request);
      GeneticAlgorithm<C> ga = new GeneticAlgorithm<>(domain, config);
      return ga.run(runId, runtime, null);
    });
    run.setMetadata(metadata);
    return run;
  }

  /**
   * Returns a run by ID.
   *
   * @param runId unique run ID
   * @return matching run or {@code null}
   */
  public SearchRun<C> get(String runId) {
    return registry.get(runId);
  }

  /**
   * Returns all known runs.
   *
   * @return current runs
   */
  public List<SearchRun<C>> list() {
    return registry.list();
  }

  @Override
  public void close() {
    registry.close();
  }

  private static Map<String, String> describeConfig(GeneticAlgorithmConfig config) {
    Map<String, String> fields = new LinkedHashMap<>();
    fields.put("populationSize", String.valueOf(config.populationSize()));
    fields.put("maxGenerations", String.valueOf(config.maxGenerations()));
    fields.put("eliteCount", String.valueOf(config.eliteCount()));
    fields.put("crossoverRate", String.valueOf(config.crossoverRate()));
    fields.put("seed", String.valueOf(config.seed()));
    return fields;
  }
}
