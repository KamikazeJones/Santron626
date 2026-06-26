package dev.quassel.ga.tsp;

import dev.quassel.ga.core.GeneticAlgorithmConfig;
import dev.quassel.ga.core.SearchDomain;
import dev.quassel.ga.runtime.RegisteredProblem;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

/** Boundary adapter that plugs the travelling-salesman problem into the runtime. */
public final class TspRuntimeAdapter implements RegisteredProblem<TspCandidate, TspRunRequest> {
  @Override
  public String problemId() {
    return "tsp";
  }

  @Override
  public TspRunRequest parseRequest(Map<String, String> params) {
    int width = intParam(params, "width", 1000);
    int height = intParam(params, "height", 1000);
    int cityCount = intParam(params, "cities", 40);
    long seed = longParam(params, "seed", 42L);
    int generations = intParam(params, "generations", 400);
    int population = intParam(params, "population", 300);
    if (width < 1 || height < 1) {
      throw new IllegalArgumentException("width and height must be >= 1");
    }
    if (cityCount < 2) {
      throw new IllegalArgumentException("cities must be >= 2");
    }
    if (generations < 1) {
      throw new IllegalArgumentException("generations must be >= 1");
    }
    if (population < 2) {
      throw new IllegalArgumentException("population must be >= 2");
    }
    return new TspRunRequest(width, height, cityCount, seed, generations, population);
  }

  @Override
  public SearchDomain<TspCandidate> createDomain(TspRunRequest request) {
    RandomGenerator rng = RandomGeneratorFactory.getDefault().create(request.seed());
    List<TspDomain.City> cities = TspDomain.randomCities(request.width(), request.height(), request.cityCount(), rng);
    return new TspDomain(cities);
  }

  @Override
  public GeneticAlgorithmConfig createConfig(TspRunRequest request) {
    return new GeneticAlgorithmConfig(
        request.population(),
        request.generations(),
        Math.max(2, request.population() / 20),
        0.85,
        request.seed()
    );
  }

  @Override
  public Map<String, String> describeRequest(TspRunRequest request) {
    Map<String, String> fields = new LinkedHashMap<>();
    fields.put("width", String.valueOf(request.width()));
    fields.put("height", String.valueOf(request.height()));
    fields.put("cities", String.valueOf(request.cityCount()));
    fields.put("seed", String.valueOf(request.seed()));
    fields.put("generations", String.valueOf(request.generations()));
    fields.put("population", String.valueOf(request.population()));
    return fields;
  }

  @Override
  public void selfTest() {
    TspRunRequest request = new TspRunRequest(100, 80, 10, 42L, 20, 30);
    TspDomain domain = (TspDomain) createDomain(request);
    TspCandidate candidate = domain.randomCandidate(RandomGeneratorFactory.getDefault().create(7L));
    if (candidate.route().size() != request.cityCount()) {
      throw new IllegalStateException("route length must equal city count");
    }
    double score = domain.evaluate(candidate).score();
    if (!(score > 0.0)) {
      throw new IllegalStateException("tour distance must be positive");
    }
  }

  private static int intParam(Map<String, String> params, String key, int defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Integer.parseInt(value);
  }

  private static long longParam(Map<String, String> params, String key, long defaultValue) {
    String value = params.get(key);
    return value == null || value.isEmpty() ? defaultValue : Long.parseLong(value);
  }
}
