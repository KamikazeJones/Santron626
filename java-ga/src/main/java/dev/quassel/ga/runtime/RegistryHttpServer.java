package dev.quassel.ga.runtime;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
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
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Central control-plane server that tracks workers and routes run requests.
 */
public final class RegistryHttpServer implements AutoCloseable {
  private final HttpServer server;
  private final int port;
  private final String bindHost;
  private final long startedAtMillis = System.currentTimeMillis();
  private final long pid = ProcessHandle.current().pid();
  private final String serverId = UUID.randomUUID().toString();
  private final Map<String, WorkerRegistration> workers = new ConcurrentHashMap<>();
  private final Map<String, String> runOwners = new ConcurrentHashMap<>();
  private final AtomicInteger nextWorkerIndex = new AtomicInteger();

  /**
   * Starts a registry server on the given host and port.
   *
   * @param bindHost network interface to bind to
   * @param port local TCP port
   * @throws IOException if the socket cannot be opened
   */
  public RegistryHttpServer(String bindHost, int port) throws IOException {
    this.bindHost = bindHost;
    this.port = port;
    this.server = HttpServer.create(new InetSocketAddress(bindHost, port), 0);
    server.createContext("/health", this::handleHealth);
    server.createContext("/server-info", this::handleServerInfo);
    server.createContext("/server-stop", this::handleServerStop);
    server.createContext("/workers", this::handleWorkers);
    server.createContext("/runs", this::handleRuns);
    server.start();
  }

  @Override
  public void close() {
    server.stop(0);
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
    sendJson(exchange, 200, "{\"ok\":true,\"message\":\"registry stopping\"}");
    shutdownAsync();
  }

  private void handleWorkers(HttpExchange exchange) throws IOException {
    String path = exchange.getRequestURI().getPath();
    String[] parts = path.split("/");
    if ((parts.length == 2 || (parts.length == 3 && parts[2].isEmpty()))
        && "GET".equals(exchange.getRequestMethod())) {
      sendJson(exchange, 200, workersJson());
      return;
    }
    if (parts.length == 3 && "register".equals(parts[2]) && "POST".equals(exchange.getRequestMethod())) {
      handleRegisterWorker(exchange);
      return;
    }
    if (parts.length == 4 && "unregister".equals(parts[3]) && "POST".equals(exchange.getRequestMethod())) {
      handleUnregisterWorker(exchange, urlDecode(parts[2]));
      return;
    }
    sendJson(exchange, 404, "{\"error\":\"not found\"}");
  }

  private void handleRuns(HttpExchange exchange) throws IOException {
    String path = exchange.getRequestURI().getPath();
    String[] parts = path.split("/");
    if (parts.length == 2 || (parts.length == 3 && parts[2].isEmpty())) {
      if ("GET".equals(exchange.getRequestMethod())) {
        sendJson(exchange, 200, aggregateRunsJson());
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
      String suffix = parts.length == 3 ? "" : "/" + parts[3];
      proxyRunRequest(exchange, runId, suffix);
      return;
    }
    sendJson(exchange, 404, "{\"error\":\"not found\"}");
  }

  private void handleRegisterWorker(HttpExchange exchange) throws IOException {
    Map<String, String> params = queryParams(exchange.getRequestURI());
    String workerId = params.get("serverId");
    String baseUrl = params.get("baseUrl");
    String problemId = params.get("problemId");
    if (workerId == null || workerId.isEmpty()
        || baseUrl == null || baseUrl.isEmpty()
        || problemId == null || problemId.isEmpty()) {
      sendJson(exchange, 400, "{\"error\":\"serverId, baseUrl and problemId are required\"}");
      return;
    }
    WorkerRegistration registration = new WorkerRegistration(
        workerId,
        problemId,
        baseUrl,
        longParam(params, "pid", 0L),
        longParam(params, "startedAtMillis", 0L),
        System.currentTimeMillis());
    workers.put(workerId, registration);
    sendJson(exchange, 200, "{\"ok\":true,\"worker\":" + workerJson(registration, isHealthy(registration)) + "}");
  }

  private void handleUnregisterWorker(HttpExchange exchange, String workerId) throws IOException {
    workers.remove(workerId);
    runOwners.entrySet().removeIf(entry -> workerId.equals(entry.getValue()));
    sendJson(exchange, 200, "{\"ok\":true}");
  }

  private void handleStartRun(HttpExchange exchange) throws IOException {
    Map<String, String> params = queryParams(exchange.getRequestURI());
    WorkerRegistration worker = selectWorker(params.get("problem"));
    if (worker == null) {
      sendJson(exchange, 503, "{\"error\":\"no healthy workers registered\"}");
      return;
    }
    String rawQuery = exchange.getRequestURI().getRawQuery();
    String path = rawQuery == null || rawQuery.isEmpty() ? "/runs" : "/runs?" + rawQuery;
    HttpResult result = fetchHttp("POST", worker.baseUrl() + path);
    String runId = extractString(result.body(), "runId");
    if (!runId.isEmpty()) {
      runOwners.put(runId, worker.serverId());
      sendJson(exchange, result.status(), injectServerIntoRun(result.body(), worker));
      return;
    }
    sendJson(exchange, 502, "{\"error\":\"worker returned invalid run response\"}");
  }

  private void proxyRunRequest(HttpExchange exchange, String runId, String suffix) throws IOException {
    WorkerRegistration worker = resolveWorkerForRun(runId);
    if (worker == null) {
      sendJson(exchange, 404, "{\"error\":\"run not found\"}");
      return;
    }
    String path = "/runs/" + urlEncode(runId) + suffix;
    HttpResult result;
    try {
      result = fetchHttp(exchange.getRequestMethod(), worker.baseUrl() + path);
    } catch (RuntimeException error) {
      sendJson(exchange, 502, "{\"error\":\"worker request failed\"}");
      return;
    }
    if (suffix.isEmpty() && result.status() == 200) {
      sendJson(exchange, 200, injectServerIntoRun(result.body(), worker));
      return;
    }
    sendJson(exchange, result.status(), result.body());
  }

  private WorkerRegistration resolveWorkerForRun(String runId) {
    String workerId = runOwners.get(runId);
    if (workerId != null) {
      WorkerRegistration worker = workers.get(workerId);
      if (worker != null) {
        return worker;
      }
    }
    for (WorkerRegistration worker : sortedWorkers()) {
      if (!isHealthy(worker)) {
        continue;
      }
      try {
        String listJson = fetchHttp("GET", worker.baseUrl() + "/runs").body();
        for (String runJson : extractRunBlocks(listJson)) {
          String candidateId = extractString(runJson, "runId");
          if (!candidateId.isEmpty()) {
            runOwners.put(candidateId, worker.serverId());
          }
          if (runId.equals(candidateId)) {
            return worker;
          }
        }
      } catch (RuntimeException error) {
        // Ignore unhealthy workers here; aggregate endpoints will surface health separately.
      }
    }
    return null;
  }

  private WorkerRegistration selectWorker(String problemId) {
    List<WorkerRegistration> healthyWorkers = sortedWorkers().stream()
        .filter(this::isHealthy)
        .filter(worker -> problemId == null || problemId.isEmpty() || problemId.equals(worker.problemId()))
        .toList();
    if (healthyWorkers.isEmpty()) {
      return null;
    }
    int index = Math.floorMod(nextWorkerIndex.getAndIncrement(), healthyWorkers.size());
    return healthyWorkers.get(index);
  }

  private String aggregateRunsJson() {
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    sb.append("\"server\":").append(serverInfoJson()).append(',');
    sb.append("\"workers\":[");
    List<WorkerRegistration> workerList = sortedWorkers();
    for (int i = 0; i < workerList.size(); i++) {
      WorkerRegistration worker = workerList.get(i);
      if (i > 0) {
        sb.append(',');
      }
      sb.append(workerJson(worker, isHealthy(worker)));
    }
    sb.append("],\"runs\":[");
    boolean firstRun = true;
    for (WorkerRegistration worker : workerList) {
      if (!isHealthy(worker)) {
        continue;
      }
      try {
        String listJson = fetchHttp("GET", worker.baseUrl() + "/runs").body();
        for (String runJson : extractRunBlocks(listJson)) {
          String runId = extractString(runJson, "runId");
          if (!runId.isEmpty()) {
            runOwners.put(runId, worker.serverId());
          }
          if (!firstRun) {
            sb.append(',');
          }
          firstRun = false;
          sb.append(injectServerIntoRun(runJson, worker));
        }
      } catch (RuntimeException error) {
        // Worker health is already reflected in /workers output.
      }
    }
    sb.append("]}");
    return sb.toString();
  }

  private List<WorkerRegistration> sortedWorkers() {
    return workers.values().stream()
        .sorted(Comparator.comparing(WorkerRegistration::serverId))
        .toList();
  }

  private boolean isHealthy(WorkerRegistration worker) {
    try {
      HttpResult result = fetchHttp("GET", worker.baseUrl() + "/health");
      return result.status() == 200 && result.body().contains("\"ok\":true");
    } catch (RuntimeException error) {
      return false;
    }
  }

  private String workersJson() {
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    sb.append("\"server\":").append(serverInfoJson()).append(',');
    sb.append("\"workers\":[");
    List<WorkerRegistration> workerList = sortedWorkers();
    for (int i = 0; i < workerList.size(); i++) {
      if (i > 0) {
        sb.append(',');
      }
      WorkerRegistration worker = workerList.get(i);
      sb.append(workerJson(worker, isHealthy(worker)));
    }
    sb.append("]}");
    return sb.toString();
  }

  private String injectServerIntoRun(String runJson, WorkerRegistration worker) {
    if (runJson == null || runJson.isEmpty() || runJson.charAt(0) != '{') {
      return runJson;
    }
    return "{\"server\":" + workerJson(worker, isHealthy(worker)) + "," + runJson.substring(1);
  }

  private String workerJson(WorkerRegistration worker, boolean healthy) {
    URI uri = URI.create(worker.baseUrl());
    String host = uri.getHost() == null ? "" : uri.getHost();
    int workerPort = uri.getPort();
    return "{"
        + "\"serverId\":\"" + jsonEscape(worker.serverId()) + "\","
        + "\"role\":\"worker\","
        + "\"problemId\":\"" + jsonEscape(worker.problemId()) + "\","
        + "\"baseUrl\":\"" + jsonEscape(worker.baseUrl()) + "\","
        + "\"host\":\"" + jsonEscape(host) + "\","
        + "\"port\":" + workerPort + ","
        + "\"pid\":" + worker.pid() + ","
        + "\"startedAtMillis\":" + worker.startedAtMillis() + ","
        + "\"registeredAtMillis\":" + worker.registeredAtMillis() + ","
        + "\"healthy\":" + healthy
        + "}";
  }

  private String serverInfoJson() {
    return "{"
        + "\"serverId\":\"" + jsonEscape(serverId) + "\","
        + "\"role\":\"registry\","
        + "\"bindHost\":\"" + jsonEscape(bindHost) + "\","
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
    }, "registry-http-server-shutdown");
    shutdownThread.setDaemon(true);
    shutdownThread.start();
  }

  private static HttpResult fetchHttp(String method, String url) {
    HttpClient client = HttpClient.newHttpClient();
    HttpRequest request = HttpRequest.newBuilder(URI.create(url))
        .method(method, HttpRequest.BodyPublishers.noBody())
        .header("Accept", "application/json")
        .build();
    try {
      HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
      return new HttpResult(response.statusCode(), response.body());
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

  private static long longParam(Map<String, String> params, String key, long defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Long.parseLong(value);
  }

  private static String extractArrayBlock(String json, String key) {
    int keyPos = json.indexOf("\"" + key + "\"");
    if (keyPos < 0) {
      return "";
    }
    int bracketStart = json.indexOf('[', keyPos);
    if (bracketStart < 0) {
      return "";
    }
    int depth = 0;
    boolean inString = false;
    boolean escaped = false;
    for (int i = bracketStart; i < json.length(); i++) {
      char ch = json.charAt(i);
      if (escaped) {
        escaped = false;
        continue;
      }
      if (ch == '\\' && inString) {
        escaped = true;
        continue;
      }
      if (ch == '"') {
        inString = !inString;
        continue;
      }
      if (inString) {
        continue;
      }
      if (ch == '[') {
        depth++;
      } else if (ch == ']') {
        depth--;
        if (depth == 0) {
          return json.substring(bracketStart + 1, i);
        }
      }
    }
    return "";
  }

  private static List<String> extractRunBlocks(String listJson) {
    String runsArray = extractArrayBlock(listJson, "runs");
    List<String> runs = new ArrayList<>();
    int depth = 0;
    boolean inString = false;
    boolean escaped = false;
    int start = -1;
    for (int i = 0; i < runsArray.length(); i++) {
      char ch = runsArray.charAt(i);
      if (escaped) {
        escaped = false;
        continue;
      }
      if (ch == '\\' && inString) {
        escaped = true;
        continue;
      }
      if (ch == '"') {
        inString = !inString;
        continue;
      }
      if (inString) {
        continue;
      }
      if (ch == '{') {
        if (depth == 0) {
          start = i;
        }
        depth++;
      } else if (ch == '}') {
        depth--;
        if (depth == 0 && start >= 0) {
          runs.add(runsArray.substring(start, i + 1));
          start = -1;
        }
      }
    }
    return runs;
  }

  private static String extractString(String json, String key) {
    Matcher matcher = Pattern.compile("\"" + Pattern.quote(key) + "\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"")
        .matcher(json);
    if (!matcher.find()) {
      return "";
    }
    return unescapeJsonString(matcher.group(1));
  }

  private static String unescapeJsonString(String value) {
    StringBuilder out = new StringBuilder();
    boolean escaped = false;
    for (int i = 0; i < value.length(); i++) {
      char ch = value.charAt(i);
      if (escaped) {
        switch (ch) {
          case 'n' -> out.append('\n');
          case 'r' -> out.append('\r');
          case 't' -> out.append('\t');
          case '"' -> out.append('"');
          case '\\' -> out.append('\\');
          default -> out.append(ch);
        }
        escaped = false;
      } else if (ch == '\\') {
        escaped = true;
      } else {
        out.append(ch);
      }
    }
    return out.toString();
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

  private record WorkerRegistration(
      String serverId,
      String problemId,
      String baseUrl,
      long pid,
      long startedAtMillis,
      long registeredAtMillis
  ) {
  }

  private record HttpResult(int status, String body) {
  }
}
