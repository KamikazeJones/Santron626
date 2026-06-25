package dev.quassel.ga.puzzle;

import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.GenerationStats;
import dev.quassel.ga.core.GeneticAlgorithm;
import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.RunResult;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

public final class PuzzleCli {
  private PuzzleCli() {
  }

  public static void main(String[] args) {
    if (args.length > 0 && "--self-test".equals(args[0])) {
      runSelfTest();
      return;
    }

    CliConfig cliConfig = parseConfig(args);
    RandomGenerator rng = RandomGeneratorFactory.getDefault().create(cliConfig.seed());
    PuzzleBoard start = cliConfig.boardOverride() != null
        ? cliConfig.boardOverride()
        : PuzzleDomain.scrambledBoard(cliConfig.size(), rng);
    PuzzleDomain domain = new PuzzleDomain(start, 4, 40);
    GeneticAlgorithmConfig config = new GeneticAlgorithmConfig(
        cliConfig.population(),
        cliConfig.generations(),
        12,
        0.70,
        cliConfig.seed()
    );

    System.out.println(cliConfig.size() * cliConfig.size() - 1 + "-puzzle start:");
    System.out.println(start);
    System.out.println();

    GeneticAlgorithm<PuzzleCandidate> ga = new GeneticAlgorithm<>(domain, config);
    RunResult<PuzzleCandidate> result = ga.run(PuzzleCli::printGeneration);

    System.out.println();
    System.out.println("Final best result after " + result.generations() + " generations:");
    System.out.println(renderResult(result.bestCandidate(), result.bestEvaluation()));
  }

  private static void printGeneration(GenerationStats<PuzzleCandidate> stats) {
    PuzzleCandidate best = stats.bestCandidate();
    System.out.printf(
        "gen=%d best=%.2f avg=%.2f solved=%s len=%d moves=%s%n",
        stats.generation(),
        stats.bestScore(),
        stats.averageScore(),
        stats.solved(),
        best.moves().size(),
        best.compact()
    );
  }

  private static String renderResult(PuzzleCandidate candidate, EvaluationResult evaluation) {
    return "score=" + evaluation.score()
        + " solved=" + evaluation.solved()
        + " len=" + candidate.moves().size()
        + System.lineSeparator()
        + evaluation.summary();
  }

  private static void runSelfTest() {
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
    System.out.println("self-test: PASS");
  }

  private static CliConfig parseConfig(String[] args) {
    int size = 5;
    long seed = 42L;
    int generations = 300;
    int population = 250;
    PuzzleBoard boardOverride = null;
    for (int i = 0; i < args.length; i++) {
      switch (args[i]) {
        case "--size" -> {
          if (i + 1 >= args.length) {
            throw new IllegalArgumentException("missing value after --size");
          }
          size = Integer.parseInt(args[++i]);
        }
        case "--seed" -> {
          if (i + 1 >= args.length) {
            throw new IllegalArgumentException("missing value after --seed");
          }
          seed = Long.parseLong(args[++i]);
        }
        case "--generations" -> {
          if (i + 1 >= args.length) {
            throw new IllegalArgumentException("missing value after --generations");
          }
          generations = Integer.parseInt(args[++i]);
        }
        case "--population" -> {
          if (i + 1 >= args.length) {
            throw new IllegalArgumentException("missing value after --population");
          }
          population = Integer.parseInt(args[++i]);
        }
        case "--board" -> {
          if (i + 1 >= args.length) {
            throw new IllegalArgumentException("missing value after --board");
          }
          String boardText = args[++i];
          int detectedSize = detectBoardSize(boardText);
          PuzzleBoard board = PuzzleBoard.parse(boardText, detectedSize);
          if (!board.isSolvable()) {
            throw new IllegalArgumentException("board is not solvable by parity");
          }
          boardOverride = board;
          size = detectedSize;
        }
        default -> throw new IllegalArgumentException(
            "usage: PuzzleCli [--self-test] | [--size N] [--seed N] [--generations N] [--population N] [--board \"...\"]");
      }
    }
    if (size < 2) {
      throw new IllegalArgumentException("size must be >= 2");
    }
    if (generations < 1) {
      throw new IllegalArgumentException("generations must be >= 1");
    }
    if (population < 2) {
      throw new IllegalArgumentException("population must be >= 2");
    }
    return new CliConfig(size, seed, generations, population, boardOverride);
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

  private record CliConfig(
      int size,
      long seed,
      int generations,
      int population,
      PuzzleBoard boardOverride
  ) {
  }
}
