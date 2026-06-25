package dev.quassel.ga.puzzle;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.GenerationStats;
import dev.quassel.ga.core.RunSnapshot;
import dev.quassel.ga.core.SearchRun;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Small local HTTP runtime for puzzle runs.
 *
 * <p>The server listens only on {@code 127.0.0.1}. It is intentionally small
 * and currently exposes run creation, listing, status and basic control.
 */
public final class PuzzleHttpServer implements AutoCloseable {
  private final HttpServer server;
  private final PuzzleService service;

  /**
   * Starts a local HTTP server bound to {@code 127.0.0.1}.
   *
   * @param port local TCP port
   * @throws IOException if the socket cannot be opened
   */
  public PuzzleHttpServer(int port) throws IOException {
    this.service = new PuzzleService();
    this.server = HttpServer.create(new InetSocketAddress("127.0.0.1", port), 0);
    server.createContext("/health", this::handleHealth);
    server.createContext("/runs", this::handleRuns);
    server.start();
  }

  @Override
  public void close() {
    server.stop(0);
    service.close();
  }

  private void handleHealth(HttpExchange exchange) throws IOException {
    if (!"GET".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    sendJson(exchange, 200, "{\"ok\":true}");
  }

  private void handleRuns(HttpExchange exchange) throws IOException {
    String path = exchange.getRequestURI().getPath();
    String[] parts = path.split("/");
    if (parts.length == 2 || (parts.length == 3 && parts[2].isEmpty())) {
      if ("GET".equals(exchange.getRequestMethod())) {
        handleListRuns(exchange);
        return;
      }
      if ("POST".equals(exchange.getRequestMethod())) {
        handleStartRun(exchange);
        return;
      }
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    if (parts.length >= 3) {
      String runId = parts[2];
      SearchRun<PuzzleCandidate> run = service.get(runId);
      if (run == null) {
        sendJson(exchange, 404, "{\"error\":\"run not found\"}");
        return;
      }
      if (parts.length == 3) {
        handleRunStatus(exchange, run);
        return;
      }
      if (parts.length == 4 && "best".equals(parts[3])) {
        handleRunBest(exchange, run);
        return;
      }
      if (parts.length == 4 && "pause".equals(parts[3])) {
        handleRunPause(exchange, run);
        return;
      }
      if (parts.length == 4 && "resume".equals(parts[3])) {
        handleRunResume(exchange, run);
        return;
      }
      if (parts.length == 4 && "stop".equals(parts[3])) {
        handleRunStop(exchange, run);
        return;
      }
    }
    sendJson(exchange, 404, "{\"error\":\"not found\"}");
  }

  private void handleListRuns(HttpExchange exchange) throws IOException {
    StringBuilder sb = new StringBuilder();
    sb.append("{\"runs\":[");
    boolean first = true;
    for (SearchRun<PuzzleCandidate> run : service.list()) {
      if (!first) {
        sb.append(',');
      }
      first = false;
      sb.append(runSummaryJson(run));
    }
    sb.append("]}");
    sendJson(exchange, 200, sb.toString());
  }

  private void handleStartRun(HttpExchange exchange) throws IOException {
    Map<String, String> params = queryParams(exchange.getRequestURI());
    int size = intParam(params, "size", 5);
    long seed = longParam(params, "seed", 42L);
    int generations = intParam(params, "generations", 300);
    int population = intParam(params, "population", 250);
    String board = params.get("board");
    try {
      SearchRun<PuzzleCandidate> run = service.start(
          new PuzzleRunRequest(size, seed, generations, population, board));
      sendJson(exchange, 200, runSummaryJson(run));
    } catch (IllegalArgumentException error) {
      sendJson(exchange, 400, "{\"error\":\"" + jsonEscape(error.getMessage()) + "\"}");
    }
  }

  private void handleRunStatus(HttpExchange exchange, SearchRun<PuzzleCandidate> run) throws IOException {
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private void handleRunBest(HttpExchange exchange, SearchRun<PuzzleCandidate> run) throws IOException {
    RunSnapshot<PuzzleCandidate> snapshot = run.latestSnapshot();
    if (snapshot == null) {
      sendJson(exchange, 200, "{\"runId\":\"" + run.runId() + "\",\"best\":null}");
      return;
    }
    GenerationStats<PuzzleCandidate> stats = snapshot.generationStats();
    EvaluationResult eval = stats.bestEvaluation();
    String json = "{"
        + "\"runId\":\"" + jsonEscape(run.runId()) + "\","
        + "\"status\":\"" + run.status() + "\","
        + "\"candidate\":\"" + jsonEscape(stats.bestCandidate().compact()) + "\","
        + "\"score\":" + eval.score() + ","
        + "\"solved\":" + eval.solved() + ","
        + "\"metrics\":" + metricsJson(eval.metrics()) + ","
        + "\"diagnostics\":" + stringListJson(eval.diagnostics()) + ","
        + "\"summary\":\"" + jsonEscape(eval.summary()) + "\""
        + "}";
    sendJson(exchange, 200, json);
  }

  private void handleRunPause(HttpExchange exchange, SearchRun<PuzzleCandidate> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestPause();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private void handleRunResume(HttpExchange exchange, SearchRun<PuzzleCandidate> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestResume();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private void handleRunStop(HttpExchange exchange, SearchRun<PuzzleCandidate> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestStop();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private String runSummaryJson(SearchRun<PuzzleCandidate> run) {
    RunSnapshot<PuzzleCandidate> snapshot = run.latestSnapshot();
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    sb.append("\"runId\":\"").append(jsonEscape(run.runId())).append("\",");
    sb.append("\"status\":\"").append(run.status()).append("\",");
    sb.append("\"startedAtMillis\":").append(run.startedAtMillis());
    if (snapshot != null) {
      GenerationStats<PuzzleCandidate> stats = snapshot.generationStats();
      sb.append(",\"snapshot\":{");
      sb.append("\"generation\":").append(stats.generation()).append(',');
      sb.append("\"bestScore\":").append(stats.bestScore()).append(',');
      sb.append("\"averageScore\":").append(stats.averageScore()).append(',');
      sb.append("\"scoreStdDev\":").append(stats.scoreStdDev()).append(',');
      sb.append("\"uniqueCandidateRatio\":").append(stats.uniqueCandidateRatio()).append(',');
      sb.append("\"solvedCount\":").append(stats.solvedCount()).append(',');
      sb.append("\"bestCandidate\":\"").append(jsonEscape(stats.bestCandidate().compact())).append("\",");
      sb.append("\"bestMetrics\":").append(metricsJson(stats.bestEvaluation().metrics())).append(',');
      sb.append("\"bestDiagnostics\":").append(stringListJson(stats.bestEvaluation().diagnostics()));
      sb.append('}');
    }
    Throwable failure = run.failure();
    if (failure != null) {
      sb.append(",\"failure\":\"").append(jsonEscape(failure.toString())).append('"');
    }
    sb.append('}');
    return sb.toString();
  }

  private static Map<String, String> queryParams(URI uri) {
    Map<String, String> params = new HashMap<>();
    String rawQuery = uri.getRawQuery();
    if (rawQuery == null || rawQuery.isEmpty()) {
      return params;
    }
    for (String pair : rawQuery.split("&")) {
      int eq = pair.indexOf('=');
      if (eq < 0) {
        params.put(urlDecode(pair), "");
      } else {
        params.put(urlDecode(pair.substring(0, eq)), urlDecode(pair.substring(eq + 1)));
      }
    }
    return params;
  }

  private static String urlDecode(String value) {
    return URLDecoder.decode(value, StandardCharsets.UTF_8);
  }

  private static int intParam(Map<String, String> params, String key, int defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Integer.parseInt(value);
  }

  private static long longParam(Map<String, String> params, String key, long defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Long.parseLong(value);
  }

  private static String metricsJson(Map<String, Double> metrics) {
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    boolean first = true;
    for (Map.Entry<String, Double> entry : metrics.entrySet()) {
      if (!first) {
        sb.append(',');
      }
      first = false;
      sb.append('"').append(jsonEscape(entry.getKey())).append("\":").append(entry.getValue());
    }
    sb.append('}');
    return sb.toString();
  }

  private static String stringListJson(List<String> values) {
    StringBuilder sb = new StringBuilder();
    sb.append('[');
    boolean first = true;
    for (String value : values) {
      if (!first) {
        sb.append(',');
      }
      first = false;
      sb.append('"').append(jsonEscape(value)).append('"');
    }
    sb.append(']');
    return sb.toString();
  }

  private static String jsonEscape(String value) {
    return value
        .replace("\\", "\\\\")
        .replace("\"", "\\\"")
        .replace("\n", "\\n")
        .replace("\r", "\\r");
  }

  private static void sendJson(HttpExchange exchange, int status, String json) throws IOException {
    byte[] bytes = json.getBytes(StandardCharsets.UTF_8);
    exchange.getResponseHeaders().set("Content-Type", "application/json; charset=utf-8");
    exchange.sendResponseHeaders(status, bytes.length);
    try (OutputStream os = exchange.getResponseBody()) {
      os.write(bytes);
    }
  }
}
