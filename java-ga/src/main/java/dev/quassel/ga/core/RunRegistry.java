package dev.quassel.ga.core;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * In-memory registry for active and finished runs.
 *
 * <p>The registry is the basis for external control and inspection. It assigns
 * run IDs, executes runs asynchronously and keeps them addressable for later
 * queries.
 *
 * @param <C> candidate type
 */
public final class RunRegistry<C> implements AutoCloseable {
  private final Map<String, SearchRun<C>> runs = new ConcurrentHashMap<>();
  private final ExecutorService executor = Executors.newCachedThreadPool();

  /** Creates an empty in-memory run registry. */
  public RunRegistry() {
  }

  /**
   * Starts a new asynchronous run and registers it under a fresh run ID.
   *
   * @param launcher callback that executes the run body
   * @return registered runtime state of the new run
   */
  public SearchRun<C> start(RunLauncher<C> launcher) {
    String runId = UUID.randomUUID().toString();
    SearchRun<C> run = new SearchRun<>(runId);
    runs.put(runId, run);
    executor.submit(() -> {
      try {
        RunResult<C> result = launcher.run(runId, run);
        run.setFinalResult(result);
      } catch (Throwable error) {
        run.markStopped();
        run.setFailure(error);
      }
    });
    return run;
  }

  /**
   * Returns one run by ID or {@code null} if none exists.
   *
   * @param runId unique run ID
   * @return matching run or {@code null}
   */
  public SearchRun<C> get(String runId) {
    return runs.get(runId);
  }

  /**
   * Returns all known runs in their current state.
   *
   * @return copy of the known runs
   */
  public List<SearchRun<C>> list() {
    return new ArrayList<>(runs.values());
  }

  @Override
  public void close() {
    executor.shutdownNow();
  }

  /**
   * Callback used by the registry-managed executor to run one search.
   *
   * @param <C> candidate type
   */
  @FunctionalInterface
  public interface RunLauncher<C> {
    /**
     * Starts or executes one run inside the registry-managed executor.
     *
     * @param runId preassigned unique run ID
     * @param run mutable runtime object for snapshots and control flags
     * @return final run result
     */
    RunResult<C> run(String runId, SearchRun<C> run);
  }
}
