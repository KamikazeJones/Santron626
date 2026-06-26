package dev.quassel.ga.app;

import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.GenerationStats;
import dev.quassel.ga.core.GeneticAlgorithm;
import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.RunResult;
import dev.quassel.ga.core.RunSnapshot;
import dev.quassel.ga.core.SearchDomain;
import dev.quassel.ga.runtime.RegisteredProblem;
import dev.quassel.ga.runtime.RegistryHttpServer;
import dev.quassel.ga.runtime.SearchRuntimeService;
import dev.quassel.ga.runtime.WorkerHttpServer;
import java.io.File;
import java.io.IOException;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.URI;
import java.net.URLEncoder;
import java.net.ServerSocket;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Generic application CLI for runtime management and local runs.
 */
public final class GaCli {
  private static final int REGISTRY_DEFAULT_PORT = 8788;
  private static final int WORKER_DEFAULT_PORT_START = 8789;
  private static final Map<String, RegisteredProblem<?, ?>> PROBLEMS = InstalledProblems.byId();

  private GaCli() {
  }

  public static void main(String[] args) {
    try {
      run(args);
    } catch (RuntimeException e) {
      System.err.println(e.getMessage());
      System.exit(1);
    }
  }

  private static void run(String[] args) {
    if (args.length > 0 && "--self-test".equals(args[0])) {
      runSelfTest(args);
      return;
    }
    if (args.length > 0 && "server".equals(args[0])) {
      launchServer(args);
      return;
    }
    if (args.length > 0 && "registry".equals(args[0])) {
      launchRegistry(args);
      return;
    }
    if (args.length > 0 && "server-foreground".equals(args[0])) {
      runServerForeground(args);
      return;
    }
    if (args.length > 0 && "registry-foreground".equals(args[0])) {
      runRegistryForeground(args);
      return;
    }
    if (args.length > 0 && "server-status".equals(args[0])) {
      runServerStatus(args);
      return;
    }
    if (args.length > 0 && "server-stop".equals(args[0])) {
      runServerStop(args);
      return;
    }
    if (args.length > 0 && "shutdown".equals(args[0])) {
      runShutdown(args);
      return;
    }
    if (args.length > 0 && "start".equals(args[0])) {
      runHttpStart(args);
      return;
    }
    if (args.length > 0 && "list".equals(args[0])) {
      runHttpGet("/runs", httpPort(args));
      return;
    }
    if (args.length > 0 && "health".equals(args[0])) {
      runHttpGet("/health", httpPort(args));
      return;
    }
    if (args.length > 0 && "server-info".equals(args[0])) {
      runHttpGet("/server-info", httpPort(args));
      return;
    }
    if (args.length > 0 && "watch".equals(args[0])) {
      if (args.length > 1 && "all".equals(args[1])) {
        runHttpWatchAll(args);
        return;
      }
      runHttpWatch(args);
      return;
    }
    if (args.length > 1 && "status".equals(args[0])) {
      runHttpGet("/runs/" + urlEncode(args[1]), httpPort(args));
      return;
    }
    if (args.length > 1 && "best".equals(args[0])) {
      runHttpGet("/runs/" + urlEncode(args[1]) + "/best", httpPort(args));
      return;
    }
    if (args.length > 1 && "pause".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/pause", httpPort(args));
      return;
    }
    if (args.length > 1 && "resume".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/resume", httpPort(args));
      return;
    }
    if (args.length > 1 && "stop".equals(args[0])) {
      runHttpPost("/runs/" + urlEncode(args[1]) + "/stop", httpPort(args));
      return;
    }
    runLocal(args);
  }

  private static void runSelfTest(String[] args) {
    RegisteredProblem<?, ?> problem = resolveProblem(args);
    problem.selfTest();
    System.out.println("self-test: PASS (" + problem.problemId() + ")");
  }

  @SuppressWarnings("unchecked")
  private static <C, R> void runLocal(String[] args) {
    RegisteredProblem<C, R> problem = resolveProblem(args);
    Map<String, String> params = parseOptionMap(args, 0, "problem");
    R request = problem.parseRequest(params);
    SearchDomain<C> domain = problem.createDomain(request);
    GeneticAlgorithmConfig config = problem.createConfig(request);
    GeneticAlgorithm<C> ga = new GeneticAlgorithm<>(domain, config);
    RunResult<C> result = ga.run(GaCli::printGeneration);
    System.out.println();
    System.out.println("problem=" + problem.problemId());
    System.out.println("Final best result after " + result.generations() + " generations:");
    System.out.println("run-id=" + result.runId());
    System.out.println(renderResult(result.bestDescription(), result.bestEvaluation()));
    printRunSummary(result);
  }

  private static void launchServer(String[] args) {
    ServerLaunchConfig config = parseServerLaunchConfig(args);
    int port = config.explicitPort() != null
        ? config.explicitPort()
        : findAvailablePort(config.bindHost(), WORKER_DEFAULT_PORT_START);
    File logFile = logFile("worker-" + config.problem().problemId(), port);
    long pid = launchBackgroundProcess(backgroundCommand(
        "server-foreground",
        "--problem", config.problem().problemId(),
        "--port", String.valueOf(port),
        "--bind-host", config.bindHost(),
        "--advertise-host", config.advertiseHost(),
        config.registryUrl() == null ? null : "--registry-url",
        config.registryUrl()),
        logFile);
    System.out.println("worker start requested for problem " + config.problem().problemId() + " on port " + port);
    System.out.println("pid=" + pid);
    System.out.println("log=" + logFile.getAbsolutePath());
  }

  @SuppressWarnings("unchecked")
  private static <C, R> void runServerForeground(String[] args) {
    ServerLaunchConfig config = parseServerLaunchConfig(args);
    int port = config.explicitPort() != null
        ? config.explicitPort()
        : findAvailablePort(config.bindHost(), WORKER_DEFAULT_PORT_START);
    SearchRuntimeService<C, R> service = new SearchRuntimeService<>(config.problem());
    try (WorkerHttpServer<C, R> server = new WorkerHttpServer<>(
        config.bindHost(),
        config.advertiseHost(),
        port,
        config.problem().problemId(),
        service,
        config.problem(),
        config.registryUrl())) {
      System.out.println("Worker HTTP server listening on http://" + config.bindHost() + ":" + port);
      Thread.currentThread().join();
    } catch (IOException e) {
      throw new RuntimeException("failed to start worker HTTP server", e);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static void launchRegistry(String[] args) {
    RegistryLaunchConfig config = parseRegistryLaunchConfig(args);
    File logFile = logFile("registry", config.port());
    long pid = launchBackgroundProcess(backgroundCommand(
        "registry-foreground",
        "--port", String.valueOf(config.port()),
        "--bind-host", config.bindHost()),
        logFile);
    System.out.println("registry start requested on port " + config.port());
    System.out.println("pid=" + pid);
    System.out.println("log=" + logFile.getAbsolutePath());
  }

  private static void runRegistryForeground(String[] args) {
    RegistryLaunchConfig config = parseRegistryLaunchConfig(args);
    try (RegistryHttpServer server = new RegistryHttpServer(config.bindHost(), config.port())) {
      System.out.println("Registry HTTP server listening on http://" + config.bindHost() + ":" + config.port());
      Thread.currentThread().join();
    } catch (IOException e) {
      throw new RuntimeException("failed to start registry HTTP server", e);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static void runServerStatus(String[] args) {
    int port = httpPort(args);
    final String health;
    try {
      health = fetchHttp("GET", "/health", port);
    } catch (RuntimeException error) {
      System.out.println("server not running");
      return;
    }
    if (!health.contains("\"ok\":true")) {
      System.out.println("server not running");
      return;
    }
    System.out.println("server running on port " + port);
    System.out.println(prettyJson(fetchHttp("GET", "/server-info", port)));
    System.out.println("runs:");
    System.out.println(prettyJson(fetchHttp("GET", "/runs", port)));
  }

  private static void runServerStop(String[] args) {
    int port = httpPort(args);
    try {
      String health = fetchHttp("GET", "/health", port);
      if (!health.contains("\"ok\":true")) {
        System.out.println("server not running");
        return;
      }
    } catch (RuntimeException error) {
      System.out.println("server not running");
      return;
    }
    System.out.println(prettyJson(fetchHttp("POST", "/server-stop", port)));
  }

  private static void runShutdown(String[] args) {
    int port = httpPort(args);
    String role = detectServerRole(port);
    if (role.isEmpty()) {
      System.out.println("registry not running");
      return;
    }
    if (!"registry".equals(role)) {
      throw new IllegalStateException("port " + port + " is not a registry port");
    }
    String workersJson = fetchHttp("GET", "/workers", port);
    List<Integer> workerPorts = new ArrayList<>();
    for (String workerJson : extractArrayObjectBlocks(workersJson, "workers")) {
      int workerPort = (int) extractNumber(workerJson, "port");
      if (workerPort > 0 && !workerPorts.contains(workerPort)) {
        workerPorts.add(workerPort);
      }
    }
    for (int workerPort : workerPorts) {
      System.out.println("stopping worker on port " + workerPort);
      try {
        System.out.println(prettyJson(fetchHttp("POST", "/server-stop", workerPort)));
      } catch (RuntimeException error) {
        System.out.println("worker on port " + workerPort + " did not respond");
      }
    }
    System.out.println("stopping registry on port " + port);
    System.out.println(prettyJson(fetchHttp("POST", "/server-stop", port)));
  }

  private static void runHttpStart(String[] args) {
    bootstrapRuntimeIfNeeded(args);
    String query = buildStartQuery(args);
    runHttpRequest("POST", "/runs" + query, httpPort(args));
  }

  private static void bootstrapRuntimeIfNeeded(String[] args) {
    int port = httpPort(args);
    String role = detectServerRole(port);
    if ("worker".equals(role)) {
      return;
    }
    if (role.isEmpty()) {
      ensureRegistryRunning(args, port);
      role = detectServerRole(port);
    }
    if (!"registry".equals(role)) {
      throw new IllegalStateException("port " + port + " is not a registry port");
    }
    ensureWorkerRunning(args, port);
  }

  private static void ensureRegistryRunning(String[] args, int port) {
    String role = detectServerRole(port);
    if ("registry".equals(role)) {
      return;
    }
    if ("worker".equals(role)) {
      throw new IllegalStateException("port " + port + " already belongs to a worker, not a registry");
    }
    String bindHost = optionValueOrDefault(args, "bind-host", "127.0.0.1");
    File logFile = logFile("registry", port);
    long pid = launchBackgroundProcess(backgroundCommand(
        "registry-foreground",
        "--port", String.valueOf(port),
        "--bind-host", bindHost),
        logFile);
    System.out.println("registry start requested on port " + port);
    System.out.println("pid=" + pid);
    System.out.println("log=" + logFile.getAbsolutePath());
    try {
      waitForServerRole(port, "registry", 10_000L);
    } catch (IllegalStateException error) {
      throw new IllegalStateException(error.getMessage() + " (siehe " + logFile.getAbsolutePath() + ")", error);
    }
  }

  private static void ensureWorkerRunning(String[] args, int registryPort) {
    RegisteredProblem<?, ?> problem = resolveProblem(args);
    if (hasHealthyWorkerForProblem(registryPort, problem.problemId())) {
      return;
    }
    String bindHost = optionValueOrDefault(args, "bind-host", "127.0.0.1");
    String advertiseHost = optionValueOrDefault(args, "advertise-host", "127.0.0.1");
    int workerPort = findAvailablePort(bindHost, WORKER_DEFAULT_PORT_START);
    File logFile = logFile("worker-" + problem.problemId(), workerPort);
    long pid = launchBackgroundProcess(backgroundCommand(
        "server-foreground",
        "--problem", problem.problemId(),
        "--port", String.valueOf(workerPort),
        "--bind-host", bindHost,
        "--advertise-host", advertiseHost,
        "--registry-url", localBaseUrl(bindHost, registryPort)),
        logFile);
    System.out.println("worker start requested for problem " + problem.problemId() + " on port " + workerPort);
    System.out.println("pid=" + pid);
    System.out.println("log=" + logFile.getAbsolutePath());
    try {
      waitForHealthyWorker(registryPort, problem.problemId(), 10_000L);
    } catch (IllegalStateException error) {
      throw new IllegalStateException(error.getMessage() + " (siehe " + logFile.getAbsolutePath() + ")", error);
    }
  }

  private static String detectServerRole(int port) {
    try {
      return extractString(fetchHttp("GET", "/server-info", port), "role");
    } catch (RuntimeException error) {
      return "";
    }
  }

  private static void waitForServerRole(int port, String expectedRole, long timeoutMillis) {
    long deadline = System.currentTimeMillis() + timeoutMillis;
    while (System.currentTimeMillis() < deadline) {
      if (expectedRole.equals(detectServerRole(port))) {
        return;
      }
      sleepQuietly(200L);
    }
    throw new IllegalStateException("server on port " + port + " did not become ready as " + expectedRole);
  }

  private static void waitForHealthyWorker(int registryPort, String problemId, long timeoutMillis) {
    long deadline = System.currentTimeMillis() + timeoutMillis;
    while (System.currentTimeMillis() < deadline) {
      if (hasHealthyWorkerForProblem(registryPort, problemId)) {
        return;
      }
      sleepQuietly(200L);
    }
    throw new IllegalStateException("no healthy worker for problem " + problemId + " registered on port " + registryPort);
  }

  private static boolean hasHealthyWorkerForProblem(int registryPort, String problemId) {
    try {
      String workersJson = fetchHttp("GET", "/workers", registryPort);
      for (String workerJson : extractArrayObjectBlocks(workersJson, "workers")) {
        if (problemId.equals(extractString(workerJson, "problemId")) && extractBoolean(workerJson, "healthy")) {
          return true;
        }
      }
      return false;
    } catch (RuntimeException error) {
      return false;
    }
  }

  private static void sleepQuietly(long millis) {
    try {
      Thread.sleep(millis);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("interrupted while waiting for runtime startup", e);
    }
  }

  private static String localBaseUrl(String bindHost, int port) {
    String host = "0.0.0.0".equals(bindHost) ? "127.0.0.1" : bindHost;
    return "http://" + host + ":" + port;
  }

  private static String buildStartQuery(String[] args) {
    Map<String, String> params = parseOptionMap(args, 1, "port", "bind-host", "advertise-host", "registry-url");
    if (!params.containsKey("problem")) {
      params.put("problem", resolveProblem(args).problemId());
    }
    return toQueryString(params);
  }

  private static void runHttpGet(String path, int port) {
    runHttpRequest("GET", path, port);
  }

  private static void runHttpPost(String path, int port) {
    runHttpRequest("POST", path, port);
  }

  private static void runHttpRequest(String method, String path, int port) {
    System.out.println(prettyJson(fetchHttp(method, path, port)));
  }

  private static void runHttpWatch(String[] args) {
    String target = null;
    long everyMillis = 1000L;
    int port = httpPort(args);
    for (int i = 1; i < args.length; i++) {
      if ("--every".equals(args[i]) && i + 1 < args.length) {
        everyMillis = Long.parseLong(args[++i]);
        continue;
      }
      if ("--port".equals(args[i]) && i + 1 < args.length) {
        i++;
        continue;
      }
      if ("--problem".equals(args[i]) && i + 1 < args.length) {
        i++;
        continue;
      }
      if (args[i].startsWith("--")) {
        throw new IllegalArgumentException("usage: GaCli watch <runId> [--every N] [--port N]");
      }
      if (target == null) {
        target = args[i];
        continue;
      }
      throw new IllegalArgumentException("usage: GaCli watch <runId> [--every N] [--port N]");
    }
    if (target == null) {
      throw new IllegalArgumentException("usage: GaCli watch <runId> [--every N] [--port N]");
    }
    if (everyMillis < 100L) {
      throw new IllegalArgumentException("--every must be >= 100");
    }
    if ("active".equals(target)) {
      watchActiveRuns(port, everyMillis);
      return;
    }
    watchSingleRun(target, port, everyMillis);
  }

  private static void watchSingleRun(String runId, int port, long everyMillis) {
    System.out.println("watching run " + runId + " every " + everyMillis + " ms on port " + port);
    boolean headerPrinted = false;
    try {
      while (true) {
        String statusJson;
        String bestJson;
        try {
          statusJson = prettyJson(fetchHttp("GET", "/runs/" + urlEncode(runId), port));
          bestJson = prettyJson(fetchHttp("GET", "/runs/" + urlEncode(runId) + "/best", port));
        } catch (RuntimeException error) {
          System.err.println("watch failed: " + error.getMessage());
          return;
        }
        printWatchDashboard(statusJson, bestJson, !headerPrinted);
        headerPrinted = true;
        if (extractString(statusJson, "status").matches("SOLVED|STOPPED")) {
          return;
        }
        Thread.sleep(everyMillis);
      }
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static void runHttpWatchAll(String[] args) {
    long everyMillis = 1000L;
    int port = httpPort(args);
    for (int i = 2; i < args.length; i++) {
      if ("--every".equals(args[i]) && i + 1 < args.length) {
        everyMillis = Long.parseLong(args[++i]);
        continue;
      }
      if ("--port".equals(args[i]) && i + 1 < args.length) {
        i++;
        continue;
      }
      throw new IllegalArgumentException("usage: GaCli watch all [--port N] [--every N]");
    }
    if (everyMillis < 100L) {
      throw new IllegalArgumentException("--every must be >= 100");
    }
    System.out.println("watching all runs every " + everyMillis + " ms on port " + port);
    try {
      while (true) {
        String listJson;
        try {
          listJson = prettyJson(fetchHttp("GET", "/runs", port));
        } catch (RuntimeException error) {
          System.err.println("watch failed: " + error.getMessage());
          return;
        }
        printAllRunsDashboard(listJson);
        Thread.sleep(everyMillis);
      }
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static void watchActiveRuns(int port, long everyMillis) {
    System.out.println("watching active runs every " + everyMillis + " ms on port " + port);
    try {
      while (true) {
        String listJson;
        try {
          listJson = prettyJson(fetchHttp("GET", "/runs", port));
        } catch (RuntimeException error) {
          System.err.println("watch failed: " + error.getMessage());
          return;
        }
        printActiveRunsDashboard(listJson);
        Thread.sleep(everyMillis);
      }
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static <C> void printGeneration(GenerationStats<C> stats) {
    System.out.printf(
        "gen=%d best=%.2f avg=%.2f solved=%s bestCandidate=%s%n",
        stats.generation(),
        stats.bestScore(),
        stats.averageScore(),
        stats.solved(),
        stats.bestDescription()
    );
  }

  private static String renderResult(String bestDescription, EvaluationResult evaluation) {
    return "score=" + evaluation.score()
        + " solved=" + evaluation.solved()
        + System.lineSeparator()
        + "best=" + bestDescription
        + System.lineSeparator()
        + "metrics=" + evaluation.metrics()
        + System.lineSeparator()
        + "diagnostics=" + evaluation.diagnostics()
        + System.lineSeparator()
        + evaluation.summary();
  }

  private static <C> void printRunSummary(RunResult<C> result) {
    RunSnapshot<C> latest = result.run().latestSnapshot();
    if (latest == null) {
      return;
    }
    GenerationStats<C> stats = latest.generationStats();
    System.out.println();
    System.out.println("Run snapshot:");
    System.out.println("status=" + latest.status()
        + " generation=" + stats.generation()
        + " bestScore=" + stats.bestScore()
        + " improvement=" + stats.bestScoreImprovement()
        + " avgScore=" + stats.averageScore()
        + " stdDev=" + stats.scoreStdDev()
        + " uniqueRatio=" + stats.uniqueCandidateRatio()
        + " stagnation=" + stats.stagnationGenerations()
        + " solvedCount=" + stats.solvedCount());
    System.out.println("averageMetrics=" + stats.averageMetrics());
    System.out.println("mutationOnlyStats=" + stats.mutationOnlyStats());
    System.out.println("crossoverMutationStats=" + stats.crossoverMutationStats());
    System.out.println("bestDescription=" + stats.bestDescription());
    System.out.println("bestMetrics=" + stats.bestEvaluation().metrics());
    System.out.println("bestDiagnostics=" + stats.bestEvaluation().diagnostics());
  }

  private static String fetchHttp(String method, String path, int port) {
    String url = "http://127.0.0.1:" + port + path;
    HttpClient client = HttpClient.newHttpClient();
    HttpRequest request = HttpRequest.newBuilder(URI.create(url))
        .method(method, HttpRequest.BodyPublishers.noBody())
        .header("Accept", "application/json")
        .build();
    try {
      HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
      return response.body();
    } catch (IOException | InterruptedException e) {
      if (hasCause(e, ConnectException.class)) {
        throw new RuntimeException(
            "cannot reach runtime at " + url
                + " - start a registry with './run.sh registry' "
                + "or run the problem locally without 'start'",
            e);
      }
      throw new RuntimeException("HTTP request failed for " + url, e);
    }
  }

  private static boolean hasCause(Throwable error, Class<? extends Throwable> type) {
    Throwable current = error;
    while (current != null) {
      if (type.isInstance(current)) {
        return true;
      }
      current = current.getCause();
    }
    return false;
  }

  private static String urlEncode(String value) {
    return URLEncoder.encode(value, StandardCharsets.UTF_8);
  }

  private static int httpPort(String[] args) {
    return httpPort(args, REGISTRY_DEFAULT_PORT);
  }

  private static int httpPort(String[] args, int defaultPort) {
    for (int i = 0; i < args.length; i++) {
      if ("--port".equals(args[i]) && i + 1 < args.length) {
        return Integer.parseInt(args[i + 1]);
      }
    }
    return defaultPort;
  }

  private static int findAvailablePort(String bindHost, int startPort) {
    for (int port = startPort; port <= 65535; port++) {
      if (isPortAvailable(bindHost, port)) {
        return port;
      }
    }
    throw new IllegalStateException("no free port available from " + startPort);
  }

  private static boolean isPortAvailable(String bindHost, int port) {
    try (ServerSocket socket = new ServerSocket(port, 50, InetAddress.getByName(bindHost))) {
      socket.setReuseAddress(true);
      return true;
    } catch (IOException e) {
      return false;
    }
  }

  private static List<String> backgroundCommand(String subcommand, String... extraArgs) {
    List<String> command = new ArrayList<>();
    command.add(System.getProperty("java.home") + File.separator + "bin" + File.separator + "java");
    command.add("-cp");
    command.add(System.getProperty("java.class.path"));
    command.add(GaCli.class.getName());
    command.add(subcommand);
    for (String arg : extraArgs) {
      if (arg != null) {
        command.add(arg);
      }
    }
    return command;
  }

  private static long launchBackgroundProcess(List<String> command, File logFile) {
    ProcessBuilder builder = new ProcessBuilder(command);
    builder.directory(new File(System.getProperty("user.dir")));
    builder.redirectErrorStream(true);
    builder.redirectOutput(ProcessBuilder.Redirect.appendTo(logFile));
    try {
      Process process = builder.start();
      return process.pid();
    } catch (IOException e) {
      throw new RuntimeException("failed to launch background process", e);
    }
  }

  private static File logFile(String prefix, int port) {
    return new File(System.getProperty("user.dir"), "." + prefix + "-" + port + ".log");
  }

  private static Map<String, String> parseOptionMap(String[] args, int startIndex, String... ignoredKeys) {
    Map<String, String> ignored = new LinkedHashMap<>();
    for (String ignoredKey : ignoredKeys) {
      ignored.put(ignoredKey, ignoredKey);
    }
    Map<String, String> params = new LinkedHashMap<>();
    for (int i = startIndex; i < args.length; i++) {
      String arg = args[i];
      if (!arg.startsWith("--")) {
        continue;
      }
      if (i + 1 >= args.length) {
        throw new IllegalArgumentException("missing value after " + arg);
      }
      String key = arg.substring(2);
      String value = args[++i];
      if (!ignored.containsKey(key)) {
        params.put(key, value);
      }
    }
    return params;
  }

  @SuppressWarnings("unchecked")
  private static <C, R> RegisteredProblem<C, R> resolveProblem(String[] args) {
    String problemId = optionValue(args, "problem");
    if (problemId == null) {
      if (PROBLEMS.size() == 1) {
        return (RegisteredProblem<C, R>) PROBLEMS.values().iterator().next();
      }
      throw new IllegalArgumentException("missing --problem");
    }
    RegisteredProblem<?, ?> problem = PROBLEMS.get(problemId);
    if (problem == null) {
      throw new IllegalArgumentException("unknown problem: " + problemId);
    }
    return (RegisteredProblem<C, R>) problem;
  }

  private static String optionValue(String[] args, String key) {
    for (int i = 0; i < args.length; i++) {
      if (("--" + key).equals(args[i]) && i + 1 < args.length) {
        return args[i + 1];
      }
    }
    return null;
  }

  private static String optionValueOrDefault(String[] args, String key, String defaultValue) {
    String value = optionValue(args, key);
    return value == null ? defaultValue : value;
  }

  private static String toQueryString(Map<String, String> params) {
    StringBuilder sb = new StringBuilder();
    boolean first = true;
    for (Map.Entry<String, String> entry : params.entrySet()) {
      sb.append(first ? '?' : '&');
      first = false;
      sb.append(urlEncode(entry.getKey())).append('=').append(urlEncode(entry.getValue()));
    }
    return sb.toString();
  }

  @SuppressWarnings("unchecked")
  private static <C, R> ServerLaunchConfig<C, R> parseServerLaunchConfig(String[] args) {
    RegisteredProblem<C, R> problem = resolveProblem(args);
    String bindHost = "127.0.0.1";
    String advertiseHost = "127.0.0.1";
    String registryUrl = null;
    Integer explicitPort = null;
    for (int i = 1; i < args.length; i++) {
      if ("--problem".equals(args[i]) && i + 1 < args.length) {
        i++;
      } else if ("--port".equals(args[i]) && i + 1 < args.length) {
        explicitPort = Integer.parseInt(args[++i]);
      } else if ("--bind-host".equals(args[i]) && i + 1 < args.length) {
        bindHost = args[++i];
      } else if ("--advertise-host".equals(args[i]) && i + 1 < args.length) {
        advertiseHost = args[++i];
      } else if ("--registry-url".equals(args[i]) && i + 1 < args.length) {
        registryUrl = args[++i];
      } else {
        throw new IllegalArgumentException(
            "usage: GaCli server --problem ID [--port N] [--bind-host HOST] [--advertise-host HOST] [--registry-url URL]");
      }
    }
    return new ServerLaunchConfig<>(problem, bindHost, advertiseHost, registryUrl, explicitPort);
  }

  private static RegistryLaunchConfig parseRegistryLaunchConfig(String[] args) {
    String bindHost = "127.0.0.1";
    int port = REGISTRY_DEFAULT_PORT;
    for (int i = 1; i < args.length; i++) {
      if ("--port".equals(args[i]) && i + 1 < args.length) {
        port = Integer.parseInt(args[++i]);
      } else if ("--bind-host".equals(args[i]) && i + 1 < args.length) {
        bindHost = args[++i];
      } else {
        throw new IllegalArgumentException("usage: GaCli registry [--port N] [--bind-host HOST]");
      }
    }
    return new RegistryLaunchConfig(bindHost, port);
  }

  private static void printWatchDashboard(String statusJson, String bestJson, boolean printHeader) {
    String status = extractString(statusJson, "status");
    int generation = (int) extractNumber(statusJson, "generation");
    double bestScore = extractNumber(statusJson, "bestScore");
    double bestScoreImprovement = extractNumber(statusJson, "bestScoreImprovement");
    double averageScore = extractNumber(statusJson, "averageScore");
    double scoreStdDev = extractNumber(statusJson, "scoreStdDev");
    double uniqueRatio = extractNumber(statusJson, "uniqueCandidateRatio");
    int stagnationGenerations = (int) extractNumber(statusJson, "stagnationGenerations");
    int solvedCount = (int) extractNumber(statusJson, "solvedCount");
    String candidate = extractString(bestJson, "candidate");
    String bestStatus = extractString(bestJson, "status");
    double score = extractNumber(bestJson, "score");
    String metricsBlock = extractObjectBlock(bestJson, "metrics");
    double totalContribution = sumMetricValues(metricsBlock);
    int maxGenerations = parseIntOrZero(extractString(statusJson, "maxGenerations"));

    System.out.println();
    if (printHeader) {
      printRunHeader(statusJson);
    }
    System.out.println("run " + extractString(statusJson, "runId")
        + " | status=" + status
        + " | gen=" + generation
        + " | best=" + format2(bestScore)
        + " | avg=" + format2(averageScore)
        + " | std=" + format2(scoreStdDev)
        + " | solved=" + solvedCount);
    System.out.println("diversity " + bar(uniqueRatio, 24) + " " + format2(uniqueRatio));
    double plateauRatio = maxGenerations <= 0 ? 0.0 : stagnationGenerations / (double) maxGenerations;
    System.out.println("plateau   " + bar(plateauRatio, 24)
        + " " + stagnationGenerations
        + " gen"
        + " | improvement=" + format2(bestScoreImprovement));
    printPopulationMetrics(statusJson, "");
    printVariationLine("mutation ", extractObjectBlock(statusJson, "mutationOnlyStats"));
    printVariationLine("crossover", extractObjectBlock(statusJson, "crossoverMutationStats"));
    System.out.println("best " + bestStatus + " | candidate=" + candidate + " | score=" + format2(score));
    if (!metricsBlock.isEmpty()) {
      System.out.println("score mix");
      for (Map.Entry<String, Double> entry : extractMetrics(metricsBlock).entrySet()) {
        System.out.println(componentLine(entry.getKey(), entry.getValue(), totalContribution));
      }
    }
  }

  private static void printAllRunsDashboard(String listJson) {
    printRunsDashboard(listJson, extractRunBlocks(listJson), "no runs");
  }

  private static void printActiveRunsDashboard(String listJson) {
    printRunsDashboard(listJson, extractActiveRunBlocks(listJson), "no active runs");
  }

  private static void printRunsDashboard(String listJson, List<String> runs, String emptyMessage) {
    String serverBlock = extractObjectBlock(listJson, "server");
    String serverId = extractString(serverBlock.isEmpty() ? listJson : serverBlock, "serverId");
    String role = extractString(serverBlock.isEmpty() ? listJson : serverBlock, "role");
    String port = serverBlock.isEmpty() ? "" : String.valueOf((int) extractNumber(serverBlock, "port"));
    System.out.println();
    if (!serverId.isEmpty()) {
      System.out.println((role.isEmpty() ? "server" : role) + " " + serverId + " on port " + port);
    } else {
      System.out.println("server on port " + port);
    }
    if (runs.isEmpty()) {
      System.out.println(emptyMessage);
      return;
    }
    for (String runJson : runs) {
      String runId = extractString(runJson, "runId");
      String status = extractString(runJson, "status");
      String snapshot = extractObjectBlock(runJson, "snapshot");
      if (snapshot.isEmpty()) {
        System.out.println(runId + " | status=" + status);
        printRunMetadataLines(runJson, "  ");
        continue;
      }
      int generation = (int) extractNumber(snapshot, "generation");
      double bestScore = extractNumber(snapshot, "bestScore");
      double averageScore = extractNumber(snapshot, "averageScore");
      double uniqueRatio = extractNumber(snapshot, "uniqueCandidateRatio");
      int stagnationGenerations = (int) extractNumber(snapshot, "stagnationGenerations");
      int solvedCount = (int) extractNumber(snapshot, "solvedCount");
      String bestCandidate = extractString(snapshot, "bestCandidate");
      String runServer = extractObjectBlock(runJson, "server");
      String runServerId = extractString(runServer, "serverId");
      int runPort = (int) extractNumber(runServer, "port");
      String problemId = extractString(runJson, "problemId");
      if (problemId.isEmpty()) {
        problemId = extractString(runServer, "problemId");
      }
      String runLocation = runServerId.isEmpty() ? "" : " | worker=" + runServerId + ":" + runPort;
      String problemLabel = problemId.isEmpty() ? "" : " | problem=" + problemId;
      System.out.println(runId
          + " | status=" + status
          + " | gen=" + generation
          + " | best=" + format2(bestScore)
          + " | avg=" + format2(averageScore)
          + " | div=" + format2(uniqueRatio)
          + " | plateau=" + stagnationGenerations
          + " | solved=" + solvedCount
          + " | candidate=" + bestCandidate
          + problemLabel
          + runLocation);
      printRunMetadataLines(runJson, "  ");
      printPopulationMetrics(runJson, "  ");
      printVariationLine("  mutation ", extractObjectBlock(runJson, "mutationOnlyStats"));
      printVariationLine("  crossover", extractObjectBlock(runJson, "crossoverMutationStats"));
    }
  }

  private static List<String> extractActiveRunBlocks(String listJson) {
    List<String> activeRuns = new ArrayList<>();
    for (String runJson : extractRunBlocks(listJson)) {
      String status = extractString(runJson, "status");
      if ("RUNNING".equals(status) || "PAUSED".equals(status)) {
        activeRuns.add(runJson);
      }
    }
    return activeRuns;
  }

  private static Map<String, Double> extractMetrics(String metricsJson) {
    Map<String, Double> metrics = new LinkedHashMap<>();
    Matcher matcher = Pattern.compile("\"((?:\\\\.|[^\"\\\\])+)\"\\s*:\\s*([-0-9.Ee+]+)").matcher(metricsJson);
    while (matcher.find()) {
      metrics.put(unescapeJsonString(matcher.group(1)), Double.parseDouble(matcher.group(2)));
    }
    return metrics;
  }

  private static double sumMetricValues(String metricsJson) {
    double total = 0.0;
    for (double value : extractMetrics(metricsJson).values()) {
      total += value;
    }
    return total;
  }

  private static void printRunHeader(String statusJson) {
    String problemId = extractString(statusJson, "problemId");
    long startedAtMillis = (long) extractNumber(statusJson, "startedAtMillis");
    String startedText = startedAtMillis > 0L ? Instant.ofEpochMilli(startedAtMillis).toString() : "";
    System.out.println("problem=" + problemId
        + (startedText.isEmpty() ? "" : " | started=" + startedText));
    printRunMetadataLines(statusJson, "");
  }

  private static void printRunMetadataLines(String json, String indent) {
    String requestBlock = extractObjectBlock(json, "request");
    if (!requestBlock.isEmpty()) {
      System.out.println(indent + "request " + formatFieldMap(extractStringMap(requestBlock)));
    }
    String configBlock = extractObjectBlock(json, "config");
    if (!configBlock.isEmpty()) {
      System.out.println(indent + "config  " + formatFieldMap(extractStringMap(configBlock)));
    }
  }

  private static void printPopulationMetrics(String json, String indent) {
    String averageMetricsBlock = extractObjectBlock(json, "averageMetrics");
    if (averageMetricsBlock.isEmpty()) {
      return;
    }
    Map<String, Double> metrics = extractMetrics(averageMetricsBlock);
    List<String> parts = new ArrayList<>();
    addMetricPart(parts, metrics, "manhattan", "avgManh");
    addMetricPart(parts, metrics, "misplaced", "avgMis");
    addMetricPart(parts, metrics, "normalizedLength", "avgLen");
    addMetricPart(parts, metrics, "ineffectiveMoves", "avgBad");
    addMetricPart(parts, metrics, "effectiveMoves", "avgGood");
    if (parts.isEmpty()) {
      for (Map.Entry<String, Double> entry : metrics.entrySet()) {
        parts.add(entry.getKey() + "=" + format2(entry.getValue()));
      }
    }
    System.out.println(indent + "population " + String.join(" ", parts));
  }

  private static void addMetricPart(List<String> parts, Map<String, Double> metrics, String key, String label) {
    Double value = metrics.get(key);
    if (value != null) {
      parts.add(label + "=" + format2(value));
    }
  }

  private static void printVariationLine(String label, String variationBlock) {
    if (variationBlock.isEmpty()) {
      return;
    }
    int attempts = (int) extractNumber(variationBlock, "attempts");
    if (attempts <= 0) {
      System.out.println(label + " " + operatorBar(0, 0, 0, 18)
          + " no offspring");
      return;
    }
    int improved = (int) extractNumber(variationBlock, "improvedCount");
    int neutral = (int) extractNumber(variationBlock, "neutralCount");
    int worsened = (int) extractNumber(variationBlock, "worsenedCount");
    double meanDelta = extractNumber(variationBlock, "meanScoreDelta");
    double bestDelta = extractNumber(variationBlock, "bestScoreDelta");
    System.out.println(label + " "
        + operatorBar(improved, neutral, worsened, 18)
        + " +" + improved
        + " =" + neutral
        + " -" + worsened
        + " | meanΔ=" + format2(meanDelta)
        + " bestΔ=" + format2(bestDelta));
  }

  private static String formatFieldMap(Map<String, String> fields) {
    StringBuilder sb = new StringBuilder();
    boolean first = true;
    for (Map.Entry<String, String> entry : fields.entrySet()) {
      if (!first) {
        sb.append(' ');
      }
      first = false;
      sb.append(entry.getKey()).append('=').append(entry.getValue());
    }
    return sb.toString();
  }

  private static Map<String, String> extractStringMap(String objectJson) {
    Map<String, String> values = new LinkedHashMap<>();
    Matcher matcher = Pattern.compile("\"((?:\\\\.|[^\"\\\\])+)\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"")
        .matcher(objectJson);
    while (matcher.find()) {
      values.put(unescapeJsonString(matcher.group(1)), unescapeJsonString(matcher.group(2)));
    }
    return values;
  }

  private static String componentLine(String name, double value, double total) {
    double ratio = total <= 0.0 ? 0.0 : value / total;
    return "  " + padRight(name, 13)
        + " " + format2(value)
        + " " + bar(ratio, 18)
        + " " + format2(ratio * 100.0) + "%";
  }

  private static String operatorBar(int improved, int neutral, int worsened, int width) {
    int total = improved + neutral + worsened;
    if (total <= 0) {
      return "[" + ".".repeat(width) + "]";
    }
    int plus = (int) Math.round(improved * width / (double) total);
    int equals = (int) Math.round(neutral * width / (double) total);
    plus = Math.min(width, plus);
    equals = Math.min(width - plus, equals);
    int minus = Math.max(0, width - plus - equals);
    StringBuilder sb = new StringBuilder(width + 2);
    sb.append('[');
    sb.append("+".repeat(plus));
    sb.append("=".repeat(equals));
    sb.append("-".repeat(minus));
    sb.append(']');
    return sb.toString();
  }

  private static String bar(double ratio, int width) {
    int filled = (int) Math.round(Math.max(0.0, Math.min(1.0, ratio)) * width);
    StringBuilder sb = new StringBuilder(width + 2);
    sb.append('[');
    for (int i = 0; i < width; i++) {
      sb.append(i < filled ? '#' : '.');
    }
    sb.append(']');
    return sb.toString();
  }

  private static String format2(double value) {
    return String.format(java.util.Locale.ROOT, "%.2f", value);
  }

  private static int parseIntOrZero(String value) {
    if (value == null || value.isEmpty()) {
      return 0;
    }
    try {
      return Integer.parseInt(value);
    } catch (NumberFormatException error) {
      return 0;
    }
  }

  private static String padRight(String value, int width) {
    if (value.length() >= width) {
      return value;
    }
    return value + " ".repeat(width - value.length());
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

  private static String extractObjectBlock(String json, String key) {
    int keyPos = json.indexOf("\"" + key + "\"");
    if (keyPos < 0) {
      return "";
    }
    int braceStart = json.indexOf('{', keyPos);
    if (braceStart < 0) {
      return "";
    }
    int depth = 0;
    for (int i = braceStart; i < json.length(); i++) {
      char ch = json.charAt(i);
      if (ch == '{') {
        depth++;
      } else if (ch == '}') {
        depth--;
        if (depth == 0) {
          return json.substring(braceStart, i + 1);
        }
      }
    }
    return "";
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

  private static List<String> extractArrayObjectBlocks(String json, String key) {
    String array = extractArrayBlock(json, key);
    List<String> blocks = new ArrayList<>();
    int depth = 0;
    boolean inString = false;
    boolean escaped = false;
    int start = -1;
    for (int i = 0; i < array.length(); i++) {
      char ch = array.charAt(i);
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
          blocks.add(array.substring(start, i + 1));
          start = -1;
        }
      }
    }
    return blocks;
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

  private static double extractNumber(String json, String key) {
    Matcher matcher = Pattern.compile("\"" + Pattern.quote(key) + "\"\\s*:\\s*([-0-9.Ee+]+)")
        .matcher(json);
    if (!matcher.find()) {
      return 0.0;
    }
    return Double.parseDouble(matcher.group(1));
  }

  private static boolean extractBoolean(String json, String key) {
    Matcher matcher = Pattern.compile("\"" + Pattern.quote(key) + "\"\\s*:\\s*(true|false)")
        .matcher(json);
    if (!matcher.find()) {
      return false;
    }
    return Boolean.parseBoolean(matcher.group(1));
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

  private record RegistryLaunchConfig(String bindHost, int port) {
  }

  private record ServerLaunchConfig<C, R>(
      RegisteredProblem<C, R> problem,
      String bindHost,
      String advertiseHost,
      String registryUrl,
      Integer explicitPort
  ) {
  }
}
