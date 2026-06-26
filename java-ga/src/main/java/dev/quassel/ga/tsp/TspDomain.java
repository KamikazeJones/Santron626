package dev.quassel.ga.tsp;

import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.SearchDomain;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.random.RandomGenerator;

/** Travelling-salesman domain on a randomly generated 2D map. */
public final class TspDomain implements SearchDomain<TspCandidate> {
  private final List<City> cities;

  /**
   * Creates a fixed-map TSP domain.
   *
   * @param cities immutable city list
   */
  public TspDomain(List<City> cities) {
    if (cities.size() < 2) {
      throw new IllegalArgumentException("cityCount must be >= 2");
    }
    this.cities = List.copyOf(cities);
  }

  @Override
  public TspCandidate randomCandidate(RandomGenerator rng) {
    List<Integer> route = new ArrayList<>(cities.size());
    for (int i = 0; i < cities.size(); i++) {
      route.add(i);
    }
    shuffle(route, rng);
    return new TspCandidate(route);
  }

  @Override
  public TspCandidate mutate(TspCandidate candidate, RandomGenerator rng) {
    List<Integer> route = new ArrayList<>(candidate.route());
    int mode = rng.nextInt(3);
    switch (mode) {
      case 0 -> swap(route, rng.nextInt(route.size()), rng.nextInt(route.size()));
      case 1 -> reverseSegment(route, rng);
      default -> reinsert(route, rng);
    }
    return new TspCandidate(route);
  }

  @Override
  public TspCandidate crossover(TspCandidate left, TspCandidate right, RandomGenerator rng) {
    int size = left.route().size();
    int start = rng.nextInt(size);
    int end = rng.nextInt(size);
    if (start > end) {
      int tmp = start;
      start = end;
      end = tmp;
    }
    List<Integer> child = new ArrayList<>(size);
    for (int i = 0; i < size; i++) {
      child.add(-1);
    }
    boolean[] used = new boolean[size];
    for (int i = start; i <= end; i++) {
      int city = left.route().get(i);
      child.set(i, city);
      used[city] = true;
    }
    int rightIndex = 0;
    for (int i = 0; i < size; i++) {
      if (child.get(i) != -1) {
        continue;
      }
      while (used[right.route().get(rightIndex)]) {
        rightIndex++;
      }
      int city = right.route().get(rightIndex++);
      child.set(i, city);
      used[city] = true;
    }
    return new TspCandidate(child);
  }

  @Override
  public EvaluationResult evaluate(TspCandidate candidate) {
    double totalDistance = totalDistance(candidate.route());
    double longestEdge = longestEdge(candidate.route());
    int crossings = crossingCount(candidate.route());
    double score = totalDistance + crossings * 5.0;
    Map<String, Double> metrics = new LinkedHashMap<>();
    metrics.put("totalDistance", totalDistance);
    metrics.put("longestEdge", longestEdge);
    metrics.put("crossings", (double) crossings);
    metrics.put("cityCount", (double) cities.size());
    List<String> diagnostics = new ArrayList<>();
    if (crossings > 0) {
      diagnostics.add("route contains self-crossings");
    }
    if (longestEdge > totalDistance / 3.0) {
      diagnostics.add("route contains a disproportionately long edge");
    }
    String summary = "distance=" + format2(totalDistance)
        + " crossings=" + crossings
        + " route=" + compactRoute(candidate.route());
    return new EvaluationResult(score, false, summary, metrics, diagnostics);
  }

  @Override
  public String describe(TspCandidate candidate) {
    EvaluationResult result = evaluate(candidate);
    return "score=" + format2(result.score())
        + " route=" + compactRoute(candidate.route())
        + " " + result.summary();
  }

  @Override
  public String fingerprint(TspCandidate candidate) {
    List<Integer> normalized = canonicalRoute(candidate.route());
    StringBuilder sb = new StringBuilder(normalized.size() * 3);
    for (int i = 0; i < normalized.size(); i++) {
      if (i > 0) {
        sb.append('-');
      }
      sb.append(normalized.get(i));
    }
    return sb.toString();
  }

  /**
   * Generates a fixed random map of cities.
   *
   * @param width map width
   * @param height map height
   * @param cityCount number of cities
   * @param rng random source
   * @return immutable city list
   */
  public static List<City> randomCities(int width, int height, int cityCount, RandomGenerator rng) {
    List<City> cities = new ArrayList<>(cityCount);
    for (int i = 0; i < cityCount; i++) {
      cities.add(new City(i, rng.nextDouble(width), rng.nextDouble(height)));
    }
    return List.copyOf(cities);
  }

  private double totalDistance(List<Integer> route) {
    double total = 0.0;
    for (int i = 0; i < route.size(); i++) {
      City a = cities.get(route.get(i));
      City b = cities.get(route.get((i + 1) % route.size()));
      total += distance(a, b);
    }
    return total;
  }

  private double longestEdge(List<Integer> route) {
    double max = 0.0;
    for (int i = 0; i < route.size(); i++) {
      City a = cities.get(route.get(i));
      City b = cities.get(route.get((i + 1) % route.size()));
      max = Math.max(max, distance(a, b));
    }
    return max;
  }

  private int crossingCount(List<Integer> route) {
    int crossings = 0;
    for (int i = 0; i < route.size(); i++) {
      City a1 = cities.get(route.get(i));
      City a2 = cities.get(route.get((i + 1) % route.size()));
      for (int j = i + 1; j < route.size(); j++) {
        int nextJ = (j + 1) % route.size();
        if (i == j || (i + 1) % route.size() == j || i == nextJ) {
          continue;
        }
        City b1 = cities.get(route.get(j));
        City b2 = cities.get(route.get(nextJ));
        if (segmentsCross(a1, a2, b1, b2)) {
          crossings++;
        }
      }
    }
    return crossings;
  }

  private static boolean segmentsCross(City a, City b, City c, City d) {
    double abC = orientation(a, b, c);
    double abD = orientation(a, b, d);
    double cdA = orientation(c, d, a);
    double cdB = orientation(c, d, b);
    return abC * abD < 0.0 && cdA * cdB < 0.0;
  }

  private static double orientation(City a, City b, City c) {
    return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
  }

  private static double distance(City a, City b) {
    double dx = a.x() - b.x();
    double dy = a.y() - b.y();
    return Math.sqrt(dx * dx + dy * dy);
  }

  private static void shuffle(List<Integer> values, RandomGenerator rng) {
    for (int i = values.size() - 1; i > 0; i--) {
      swap(values, i, rng.nextInt(i + 1));
    }
  }

  private static void swap(List<Integer> values, int a, int b) {
    Integer tmp = values.get(a);
    values.set(a, values.get(b));
    values.set(b, tmp);
  }

  private static void reverseSegment(List<Integer> route, RandomGenerator rng) {
    int a = rng.nextInt(route.size());
    int b = rng.nextInt(route.size());
    if (a > b) {
      int tmp = a;
      a = b;
      b = tmp;
    }
    while (a < b) {
      swap(route, a++, b--);
    }
  }

  private static void reinsert(List<Integer> route, RandomGenerator rng) {
    int from = rng.nextInt(route.size());
    int to = rng.nextInt(route.size());
    Integer city = route.remove(from);
    route.add(to, city);
  }

  private static List<Integer> canonicalRoute(List<Integer> route) {
    int pivot = 0;
    for (int i = 1; i < route.size(); i++) {
      if (route.get(i) < route.get(pivot)) {
        pivot = i;
      }
    }
    List<Integer> forward = rotate(route, pivot);
    List<Integer> backward = rotate(reverse(route), route.size() - 1 - pivot);
    return lexicographicallyLessOrEqual(forward, backward) ? forward : backward;
  }

  private static List<Integer> rotate(List<Integer> route, int pivot) {
    List<Integer> rotated = new ArrayList<>(route.size());
    for (int i = 0; i < route.size(); i++) {
      rotated.add(route.get((pivot + i) % route.size()));
    }
    return rotated;
  }

  private static List<Integer> reverse(List<Integer> route) {
    List<Integer> reversed = new ArrayList<>(route);
    java.util.Collections.reverse(reversed);
    return reversed;
  }

  private static boolean lexicographicallyLessOrEqual(List<Integer> left, List<Integer> right) {
    for (int i = 0; i < left.size(); i++) {
      int cmp = Integer.compare(left.get(i), right.get(i));
      if (cmp != 0) {
        return cmp < 0;
      }
    }
    return true;
  }

  private static String compactRoute(List<Integer> route) {
    StringBuilder sb = new StringBuilder();
    int preview = Math.min(route.size(), 12);
    for (int i = 0; i < preview; i++) {
      if (i > 0) {
        sb.append("->");
      }
      sb.append(route.get(i));
    }
    if (route.size() > preview) {
      sb.append("->...");
    }
    return sb.toString();
  }

  private static String format2(double value) {
    return String.format(java.util.Locale.ROOT, "%.2f", value);
  }

  /** One fixed city on the TSP map. */
  public record City(int id, double x, double y) {
  }
}
