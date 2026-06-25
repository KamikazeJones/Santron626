package dev.quassel.ga.puzzle;

import dev.quassel.ga.core.GeneticAlgorithm;
import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.RunRegistry;
import dev.quassel.ga.core.SearchRun;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

/**
 * Application service for puzzle runs.
 *
 * <p>This wraps the generic core types in a concrete puzzle-specific runtime
 * that can be used by HTTP or CLI frontends.
 */
public final class PuzzleService implements AutoCloseable {
  private final RunRegistry<PuzzleCandidate> registry = new RunRegistry<>();

  /** Creates a puzzle service with its own in-memory run registry. */
  public PuzzleService() {
  }

  /**
   * Starts one asynchronous puzzle run.
   *
   * @param request run parameters
   * @return runtime object for the new run
   */
  public SearchRun<PuzzleCandidate> start(PuzzleRunRequest request) {
    return registry.start((runId, run) -> {
      PuzzleBoard startBoard = resolveStartBoard(request);
      PuzzleDomain domain = new PuzzleDomain(startBoard, 4, 40);
      GeneticAlgorithmConfig config = new GeneticAlgorithmConfig(
          request.population(),
          request.generations(),
          12,
          0.70,
          request.seed()
      );
      GeneticAlgorithm<PuzzleCandidate> ga = new GeneticAlgorithm<>(domain, config);
      return ga.run(runId, run, null);
    });
  }

  /**
   * Returns a run by ID.
   *
   * @param runId unique run ID
   * @return matching run or {@code null}
   */
  public SearchRun<PuzzleCandidate> get(String runId) {
    return registry.get(runId);
  }

  /**
   * Returns all known puzzle runs.
   *
   * @return run list
   */
  public java.util.List<SearchRun<PuzzleCandidate>> list() {
    return registry.list();
  }

  @Override
  public void close() {
    registry.close();
  }

  private static PuzzleBoard resolveStartBoard(PuzzleRunRequest request) {
    if (request.boardText() != null) {
      int detectedSize = PuzzleCli.detectBoardSize(request.boardText());
      PuzzleBoard board = PuzzleBoard.parse(request.boardText(), detectedSize);
      if (!board.isSolvable()) {
        throw new IllegalArgumentException("board is not solvable by parity");
      }
      return board;
    }
    RandomGenerator rng = RandomGeneratorFactory.getDefault().create(request.seed());
    return PuzzleDomain.scrambledBoard(request.size(), rng);
  }
}
