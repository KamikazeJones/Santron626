package dev.quassel.ga.runtime;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.GenerationStats;
import dev.quassel.ga.core.RunMetadata;
import dev.quassel.ga.core.RunSnapshot;
import dev.quassel.ga.core.SearchRun;
import dev.quassel.ga.core.VariationStats;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.URLEncoder;
import java.net.URLDecoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Generic HTTP worker that exposes runs for one problem adapter.
 *
 * @param <C> candidate type
 * @param <R> request/specification type
 */
public final class WorkerHttpServer<C, R> implements AutoCloseable {
  private final HttpServer server;
  private final SearchRuntimeService<C, R> service;
  private final StartRequestParser<R> requestParser;
  private final int port;
  private final String problemId;
  private final String bindHost;
  private final String advertiseHost;
  private final String registryBaseUrl;
  private final long startedAtMillis = System.currentTimeMillis();
  private final long pid = ProcessHandle.current().pid();
  private final String serverId = UUID.randomUUID().toString();
  private volatile boolean closing;
  private volatile boolean registeredWithRegistry;

  /**
   * Starts a worker on the given host and port.
   *
   * @param bindHost host/interface to bind to
   * @param advertiseHost host name announced to the registry
   * @param port local TCP port
   * @param problemId stable problem identifier
   * @param service generic runtime service
   * @param requestParser transport-to-request parser
   * @param registryBaseUrl optional registry base URL
   * @throws IOException if the socket cannot be opened
   */
  public WorkerHttpServer(
      String bindHost,
      String advertiseHost,
      int port,
      String problemId,
      SearchRuntimeService<C, R> service,
      StartRequestParser<R> requestParser,
      String registryBaseUrl) throws IOException {
    this.bindHost = bindHost;
    this.advertiseHost = advertiseHost;
    this.port = port;
    this.problemId = problemId;
    this.service = service;
    this.requestParser = requestParser;
    this.registryBaseUrl = registryBaseUrl;
    this.server = HttpServer.create(new InetSocketAddress(bindHost, port), 0);
    server.createContext("/health", this::handleHealth);
    server.createContext("/server-info", this::handleServerInfo);
    server.createContext("/server-stop", this::handleServerStop);
    server.createContext("/runs", this::handleRuns);
    server.start();
    startRegistryRegistrationLoop();
  }

  @Override
  public void close() {
    closing = true;
    unregisterFromRegistry();
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

  private void handleServerInfo(HttpExchange exchange) throws IOException {
    if (!"GET".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    sendJson(exchange, 200, serverInfoJson());
  }

  private void handleServerStop(HttpExchange exchange) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    sendJson(exchange, 200, "{\"ok\":true,\"message\":\"server stopping\"}");
    shutdownAsync();
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
      String runId = urlDecode(parts[2]);
      SearchRun<C> run = service.get(runId);
      if (run == null) {
        sendJson(exchange, 404, "{\"error\":\"run not found\"}");
        return;
      }
      if (parts.length == 3) {
        sendJson(exchange, 200, runSummaryJson(run));
        return;
      }
      if (parts.length == 4 && "best".equals(parts[3])) {
        sendJson(exchange, 200, runBestJson(run));
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
    sb.append('{');
    sb.append("\"server\":").append(serverInfoJson()).append(',');
    sb.append("\"runs\":[");
    boolean first = true;
    for (SearchRun<C> run : service.list()) {
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
    try {
      R request = requestParser.parseRequest(params);
      SearchRun<C> run = service.start(request);
      sendJson(exchange, 200, runSummaryJson(run));
    } catch (IllegalArgumentException error) {
      sendJson(exchange, 400, "{\"error\":\"" + jsonEscape(error.getMessage()) + "\"}");
    }
  }

  private void handleRunPause(HttpExchange exchange, SearchRun<C> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestPause();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private void handleRunResume(HttpExchange exchange, SearchRun<C> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestResume();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private void handleRunStop(HttpExchange exchange, SearchRun<C> run) throws IOException {
    if (!"POST".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 405, "{\"error\":\"method not allowed\"}");
      return;
    }
    run.requestStop();
    sendJson(exchange, 200, runSummaryJson(run));
  }

  private String runSummaryJson(SearchRun<C> run) {
    RunSnapshot<C> snapshot = run.latestSnapshot();
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    sb.append("\"runId\":\"").append(jsonEscape(run.runId())).append("\",");
    sb.append("\"problemId\":\"").append(jsonEscape(problemId)).append("\",");
    sb.append("\"status\":\"").append(run.status()).append("\",");
    sb.append("\"startedAtMillis\":").append(run.startedAtMillis());
    RunMetadata metadata = run.metadata();
    if (metadata != null) {
      sb.append(",\"request\":").append(stringMapJson(metadata.request()));
      sb.append(",\"config\":").append(stringMapJson(metadata.config()));
    }
    if (snapshot != null) {
      GenerationStats<C> stats = snapshot.generationStats();
      sb.append(",\"snapshot\":{");
      sb.append("\"generation\":").append(stats.generation()).append(',');
      sb.append("\"bestScore\":").append(stats.bestScore()).append(',');
      sb.append("\"bestScoreImprovement\":").append(stats.bestScoreImprovement()).append(',');
      sb.append("\"averageScore\":").append(stats.averageScore()).append(',');
      sb.append("\"scoreStdDev\":").append(stats.scoreStdDev()).append(',');
      sb.append("\"uniqueCandidateRatio\":").append(stats.uniqueCandidateRatio()).append(',');
      sb.append("\"stagnationGenerations\":").append(stats.stagnationGenerations()).append(',');
      sb.append("\"solvedCount\":").append(stats.solvedCount()).append(',');
      sb.append("\"bestCandidate\":\"").append(jsonEscape(stats.bestDescription())).append("\",");
      sb.append("\"averageMetrics\":").append(metricsJson(stats.averageMetrics())).append(',');
      sb.append("\"mutationOnlyStats\":").append(variationStatsJson(stats.mutationOnlyStats())).append(',');
      sb.append("\"crossoverMutationStats\":").append(variationStatsJson(stats.crossoverMutationStats())).append(',');
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

  private String runBestJson(SearchRun<C> run) {
    RunSnapshot<C> snapshot = run.latestSnapshot();
    if (snapshot == null) {
      return "{\"runId\":\"" + jsonEscape(run.runId()) + "\",\"best\":null}";
    }
    GenerationStats<C> stats = snapshot.generationStats();
    EvaluationResult eval = stats.bestEvaluation();
    return "{"
        + "\"runId\":\"" + jsonEscape(run.runId()) + "\","
        + "\"status\":\"" + run.status() + "\","
        + "\"candidate\":\"" + jsonEscape(stats.bestDescription()) + "\","
        + "\"score\":" + eval.score() + ","
        + "\"solved\":" + eval.solved() + ","
        + "\"metrics\":" + metricsJson(eval.metrics()) + ","
        + "\"diagnostics\":" + stringListJson(eval.diagnostics()) + ","
        + "\"summary\":\"" + jsonEscape(eval.summary()) + "\""
        + "}";
  }

  private void startRegistryRegistrationLoop() {
    if (registryBaseUrl == null || registryBaseUrl.isBlank()) {
      return;
    }
    Thread registrationThread = new Thread(() -> {
      while (!closing && !registeredWithRegistry) {
        registeredWithRegistry = tryRegisterWithRegistry();
        if (registeredWithRegistry) {
          return;
        }
        try {
          Thread.sleep(1000L);
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
          return;
        }
      }
    }, "worker-registry-register");
    registrationThread.setDaemon(true);
    registrationThread.start();
  }

  private boolean tryRegisterWithRegistry() {
    String path = registryBaseUrl
        + "/workers/register?serverId=" + urlEncode(serverId)
        + "&baseUrl=" + urlEncode(baseUrl())
        + "&problemId=" + urlEncode(problemId)
        + "&pid=" + pid
        + "&startedAtMillis=" + startedAtMillis;
    try {
      fetchHttp("POST", path);
      return true;
    } catch (RuntimeException error) {
      return false;
    }
  }

  private void unregisterFromRegistry() {
    if (registryBaseUrl == null || registryBaseUrl.isBlank() || !registeredWithRegistry) {
      return;
    }
    String path = registryBaseUrl + "/workers/" + urlEncode(serverId) + "/unregister";
    try {
      fetchHttp("POST", path);
    } catch (RuntimeException error) {
      // Registry unavailability during shutdown is not fatal for worker cleanup.
    }
  }

  private String baseUrl() {
    return "http://" + advertiseHost + ":" + port;
  }

  private String serverInfoJson() {
    return "{"
        + "\"serverId\":\"" + jsonEscape(serverId) + "\","
        + "\"role\":\"worker\","
        + "\"problemId\":\"" + jsonEscape(problemId) + "\","
        + "\"bindHost\":\"" + jsonEscape(bindHost) + "\","
        + "\"advertiseHost\":\"" + jsonEscape(advertiseHost) + "\","
        + "\"port\":" + port + ","
        + "\"pid\":" + pid + ","
        + "\"startedAtMillis\":" + startedAtMillis
        + "}";
  }

  private void shutdownAsync() {
    Thread shutdownThread = new Thread(() -> {
      try {
        Thread.sleep(50L);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
      try {
        close();
      } finally {
        System.exit(0);
      }
    }, "worker-http-server-shutdown");
    shutdownThread.setDaemon(true);
    shutdownThread.start();
  }

  private static String fetchHttp(String method, String url) {
    HttpClient client = HttpClient.newHttpClient();
    HttpRequest request = HttpRequest.newBuilder(URI.create(url))
        .method(method, HttpRequest.BodyPublishers.noBody())
        .header("Accept", "application/json")
        .build();
    try {
      HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
      return response.body();
    } catch (IOException | InterruptedException e) {
      throw new RuntimeException("HTTP request failed", e);
    }
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

  private static String urlEncode(String value) {
    return URLEncoder.encode(value, StandardCharsets.UTF_8);
  }

  private static String urlDecode(String value) {
    return URLDecoder.decode(value, StandardCharsets.UTF_8);
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

  private static String stringMapJson(Map<String, String> values) {
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    boolean first = true;
    for (Map.Entry<String, String> entry : values.entrySet()) {
      if (!first) {
        sb.append(',');
      }
      first = false;
      sb.append('"').append(jsonEscape(entry.getKey())).append("\":\"")
          .append(jsonEscape(entry.getValue())).append('"');
    }
    sb.append('}');
    return sb.toString();
  }

  private static String variationStatsJson(VariationStats stats) {
    return "{"
        + "\"attempts\":" + stats.attempts() + ","
        + "\"improvedCount\":" + stats.improvedCount() + ","
        + "\"neutralCount\":" + stats.neutralCount() + ","
        + "\"worsenedCount\":" + stats.worsenedCount() + ","
        + "\"meanScoreDelta\":" + stats.meanScoreDelta() + ","
        + "\"bestScoreDelta\":" + stats.bestScoreDelta()
        + "}";
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
