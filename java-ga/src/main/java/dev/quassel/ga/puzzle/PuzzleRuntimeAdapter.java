package dev.quassel.ga.puzzle;

import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.SearchDomain;
import dev.quassel.ga.runtime.RegisteredProblem;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

/**
 * Boundary adapter that plugs the sliding puzzle into the generic runtime.
 */
public final class PuzzleRuntimeAdapter
    implements RegisteredProblem<PuzzleCandidate, PuzzleRunRequest> {

  @Override
  public String problemId() {
    return "puzzle";
  }

  @Override
  public PuzzleRunRequest parseRequest(Map<String, String> params) {
    int size = intParam(params, "size", 5);
    long seed = longParam(params, "seed", 42L);
    int generations = intParam(params, "generations", 300);
    int population = intParam(params, "population", 250);
    String boardText = emptyToNull(params.get("board"));
    if (boardText != null) {
      int detectedSize = detectBoardSize(boardText);
      PuzzleBoard board = PuzzleBoard.parse(boardText, detectedSize);
      if (!board.isSolvable()) {
        throw new IllegalArgumentException("board is not solvable by parity");
      }
      size = detectedSize;
    }
    int minLength = intParam(params, "min-length", 0);
    int maxLength = intParam(params, "max-length", defaultMaxLength(size));
    if (size < 2) {
      throw new IllegalArgumentException("size must be >= 2");
    }
    if (generations < 1) {
      throw new IllegalArgumentException("generations must be >= 1");
    }
    if (population < 2) {
      throw new IllegalArgumentException("population must be >= 2");
    }
    if (minLength < 0) {
      throw new IllegalArgumentException("min-length must be >= 0");
    }
    if (maxLength < minLength) {
      throw new IllegalArgumentException("max-length must be >= min-length");
    }
    return new PuzzleRunRequest(size, seed, generations, population, minLength, maxLength, boardText);
  }

  @Override
  public SearchDomain<PuzzleCandidate> createDomain(PuzzleRunRequest request) {
    return new PuzzleDomain(resolveStartBoard(request), request.minLength(), request.maxLength());
  }

  @Override
  public GeneticAlgorithmConfig createConfig(PuzzleRunRequest request) {
    return new GeneticAlgorithmConfig(
        request.population(),
        request.generations(),
        12,
        0.70,
        request.seed()
    );
  }

  @Override
  public Map<String, String> describeRequest(PuzzleRunRequest request) {
    Map<String, String> fields = new LinkedHashMap<>();
    fields.put("size", String.valueOf(request.size()));
    fields.put("seed", String.valueOf(request.seed()));
    fields.put("generations", String.valueOf(request.generations()));
    fields.put("population", String.valueOf(request.population()));
    fields.put("minLength", String.valueOf(request.minLength()));
    fields.put("maxLength", String.valueOf(request.maxLength()));
    if (request.boardText() != null) {
      fields.put("board", request.boardText());
    }
    return fields;
  }

  @Override
  public void selfTest() {
    PuzzleBoard.selfTest();
    PuzzleBoard start = PuzzleDomain.scrambledBoard(
        5, RandomGeneratorFactory.getDefault().create(42L));
    PuzzleDomain domain = new PuzzleDomain(start, 4, 12);
    PuzzleCandidate empty = PuzzleCandidate.empty();
    EvaluationResult result = domain.evaluate(empty);
    if (result.solved()) {
      throw new IllegalStateException("scrambled start must not already be solved");
    }
    PuzzleCandidate candidate = new PuzzleCandidate(java.util.List.of(
        Move.U, Move.U, Move.L, Move.D
    ));
    domain.evaluate(candidate);
  }

  private static PuzzleBoard resolveStartBoard(PuzzleRunRequest request) {
    if (request.boardText() != null) {
      int detectedSize = detectBoardSize(request.boardText());
      return PuzzleBoard.parse(request.boardText(), detectedSize);
    }
    RandomGenerator rng = RandomGeneratorFactory.getDefault().create(request.seed());
    return PuzzleDomain.scrambledBoard(request.size(), rng);
  }

  private static int detectBoardSize(String boardText) {
    String normalized = boardText.trim().replace('/', ' ').replace(',', ' ');
    String[] parts = normalized.split("\\s+");
    int count = parts.length;
    int size = (int) Math.round(Math.sqrt(count));
    if (size * size != count) {
      throw new IllegalArgumentException("board must contain a square number of tiles");
    }
    return size;
  }

  private static int intParam(Map<String, String> params, String key, int defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Integer.parseInt(value);
  }

  private static long longParam(Map<String, String> params, String key, long defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Long.parseLong(value);
  }

  private static String emptyToNull(String value) {
    return value == null || value.isEmpty() ? null : value;
  }

  private static int defaultMaxLength(int size) {
    if (size <= 3) {
      return 31;
    }
    if (size == 4) {
      return 80;
    }
    return 12 * size * size;
  }
}
