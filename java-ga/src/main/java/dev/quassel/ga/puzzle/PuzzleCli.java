package dev.quassel.ga.puzzle;

import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.GenerationStats;
import dev.quassel.ga.core.GeneticAlgorithm;
import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.RunResult;
import dev.quassel.ga.core.RunSnapshot;
import java.io.IOException;
import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;
import java.nio.charset.StandardCharsets;

/**
 * Command-line entry point for local puzzle runs and for interacting with the
 * local HTTP runtime.
 */
public final class PuzzleCli {
  private PuzzleCli() {
  }

  /**
   * Entry point for local puzzle execution, runtime server mode and HTTP client
   * subcommands.
   *
   * @param args command-line arguments
   */
  public static void main(String[] args) {
    if (args.length > 0 && "--self-test".equals(args[0])) {
      runSelfTest();
      return;
    }
    if (args.length > 0 && "server".equals(args[0])) {
      runServer(args);
      return;
    }
    if (args.length > 0 && "start".equals(args[0])) {
      runHttpStart(args);
      return;
    }
    if (args.length > 0 && "list".equals(args[0])) {
      runHttpGet("/runs");
      return;
    }
    if (args.length > 1 && "status".equals(args[0])) {
      runHttpGet("/runs/" + urlEncode(args[1]));
      return;
    }
    if (args.length > 1 && "best".equals(args[0])) {
      runHttpGet("/runs/" + urlEncode(args[1]) + "/best");
      return;
    }
    if (args.length > 1 && "pause".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/pause");
      return;
    }
    if (args.length > 1 && "resume".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/resume");
      return;
    }
    if (args.length > 1 && "stop".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/stop");
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
    System.out.println("run-id=" + result.runId());
    System.out.println(renderResult(result.bestCandidate(), result.bestEvaluation()));
    printRunSummary(result);
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
        + "metrics=" + evaluation.metrics()
        + System.lineSeparator()
        + "diagnostics=" + evaluation.diagnostics()
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

  private static void printRunSummary(RunResult<PuzzleCandidate> result) {
    RunSnapshot<PuzzleCandidate> latest = result.run().latestSnapshot();
    if (latest == null) {
      return;
    }
    GenerationStats<PuzzleCandidate> stats = latest.generationStats();
    System.out.println();
    System.out.println("Run snapshot:");
    System.out.println("status=" + latest.status()
        + " generation=" + stats.generation()
        + " bestScore=" + stats.bestScore()
        + " avgScore=" + stats.averageScore()
        + " stdDev=" + stats.scoreStdDev()
        + " uniqueRatio=" + stats.uniqueCandidateRatio()
        + " solvedCount=" + stats.solvedCount());
    System.out.println("bestMetrics=" + stats.bestEvaluation().metrics());
    System.out.println("bestDiagnostics=" + stats.bestEvaluation().diagnostics());
  }

  static int detectBoardSize(String boardText) {
    String normalized = boardText.trim().replace('/', ' ').replace(',', ' ');
    String[] parts = normalized.split("\\s+");
    int count = parts.length;
    int size = (int) Math.round(Math.sqrt(count));
    if (size * size != count) {
      throw new IllegalArgumentException("board must contain a square number of tiles");
    }
    return size;
  }

  private static void runServer(String[] args) {
    int port = 8787;
    for (int i = 1; i < args.length; i++) {
      if ("--port".equals(args[i]) && i + 1 < args.length) {
        port = Integer.parseInt(args[++i]);
      } else {
        throw new IllegalArgumentException("usage: PuzzleCli server [--port N]");
      }
    }
    try (PuzzleHttpServer server = new PuzzleHttpServer(port)) {
      System.out.println("Puzzle HTTP server listening on http://127.0.0.1:" + port);
      Thread.currentThread().join();
    } catch (IOException e) {
      throw new RuntimeException("failed to start HTTP server", e);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static void runHttpStart(String[] args) {
    String query = buildStartQuery(args);
    runHttpRequest("POST", "/runs" + query);
  }

  private static String buildStartQuery(String[] args) {
    StringBuilder sb = new StringBuilder();
    boolean first = true;
    for (int i = 1; i < args.length; i++) {
      String key = args[i];
      if (!key.startsWith("--") || i + 1 >= args.length) {
        throw new IllegalArgumentException(
            "usage: PuzzleCli start [--size N] [--seed N] [--generations N] [--population N] [--board \"...\"] [--port N]");
      }
      String value = args[++i];
      String normalizedKey = key.substring(2);
      if ("port".equals(normalizedKey)) {
        continue;
      }
      sb.append(first ? '?' : '&');
      first = false;
      sb.append(urlEncode(normalizedKey)).append('=').append(urlEncode(value));
    }
    return sb.toString();
  }

  private static void runHttpGet(String path) {
    runHttpRequest("GET", path);
  }

  private static void runHttpPost(String path) {
    runHttpRequest("POST", path);
  }

  private static void runHttpRequest(String method, String path) {
    int port = 8787;
    HttpClient client = HttpClient.newHttpClient();
    HttpRequest request = HttpRequest.newBuilder(
            URI.create("http://127.0.0.1:" + port + path))
        .method(method, HttpRequest.BodyPublishers.noBody())
        .header("Accept", "application/json")
        .build();
    try {
      HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
      System.out.println(prettyJson(response.body()));
    } catch (IOException | InterruptedException e) {
      throw new RuntimeException("HTTP request failed", e);
    }
  }

  private static String urlEncode(String value) {
    return URLEncoder.encode(value, StandardCharsets.UTF_8);
  }

  private static String prettyJson(String json) {
    StringBuilder out = new StringBuilder();
    int indent = 0;
    boolean inString = false;
    boolean escaped = false;
    for (int i = 0; i < json.length(); i++) {
      char ch = json.charAt(i);
      if (escaped) {
        out.append(ch);
        escaped = false;
        continue;
      }
      if (ch == '\\') {
        out.append(ch);
        if (inString) {
          escaped = true;
        }
        continue;
      }
      if (ch == '"') {
        inString = !inString;
        out.append(ch);
        continue;
      }
      if (inString) {
        out.append(ch);
        continue;
      }
      switch (ch) {
        case '{', '[' -> {
          out.append(ch).append('\n');
          indent++;
          appendIndent(out, indent);
        }
        case '}', ']' -> {
          out.append('\n');
          indent--;
          appendIndent(out, indent);
          out.append(ch);
        }
        case ',' -> {
          out.append(ch).append('\n');
          appendIndent(out, indent);
        }
        case ':' -> out.append(": ");
        default -> {
          if (!Character.isWhitespace(ch)) {
            out.append(ch);
          }
        }
      }
    }
    return out.toString();
  }

  private static void appendIndent(StringBuilder out, int indent) {
    out.append("  ".repeat(Math.max(0, indent)));
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

  private record CliConfig(
      int size,
      long seed,
      int generations,
      int population,
      PuzzleBoard boardOverride
  ) {
  }
}
