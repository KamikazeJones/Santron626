package dev.quassel.ga.app;

import dev.quassel.ga.puzzle.PuzzleRuntimeAdapter;
import dev.quassel.ga.runtime.RegisteredProblem;
import dev.quassel.ga.tsp.TspRuntimeAdapter;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Application composition root for installed problem adapters.
 */
public final class InstalledProblems {
  private InstalledProblems() {
  }

  /**
   * Returns installed problems by stable ID.
   *
   * @return immutable map of installed problems
   */
  public static Map<String, RegisteredProblem<?, ?>> byId() {
    Map<String, RegisteredProblem<?, ?>> problems = new LinkedHashMap<>();
    register(problems, new PuzzleRuntimeAdapter());
    register(problems, new TspRuntimeAdapter());
    return Map.copyOf(problems);
  }

  private static void register(Map<String, RegisteredProblem<?, ?>> problems, RegisteredProblem<?, ?> problem) {
    RegisteredProblem<?, ?> previous = problems.put(problem.problemId(), problem);
    if (previous != null) {
      throw new IllegalStateException("duplicate problem ID: " + problem.problemId());
    }
  }
}
