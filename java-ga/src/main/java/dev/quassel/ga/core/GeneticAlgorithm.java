package dev.quassel.ga.core;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

/**
 * Minimal generic genetic algorithm implementation.
 *
 * <p>This class is deliberately small. It provides a baseline engine plus the
 * runtime hooks needed for observation and external control.
 *
 * @param <C> candidate type
 */
public final class GeneticAlgorithm<C> {
  private final SearchDomain<C> domain;
  private final GeneticAlgorithmConfig config;
  private final RandomGenerator rng;

  /**
   * Creates a GA instance for one domain and one static configuration.
   *
   * @param domain problem-specific search contract
   * @param config immutable run configuration
   */
  public GeneticAlgorithm(SearchDomain<C> domain, GeneticAlgorithmConfig config) {
    this.domain = domain;
    this.config = config;
    this.rng = RandomGeneratorFactory.getDefault().create(config.seed());
  }

  /**
   * Runs the GA synchronously with an internally created run object.
   *
   * @param observer optional per-generation observer
   * @return final run result
   */
  public RunResult<C> run(GenerationObserver<C> observer) {
    String runId = UUID.randomUUID().toString();
    SearchRun<C> run = new SearchRun<>(runId);
    return run(runId, run, observer);
  }

  /**
   * Runs the GA against an externally supplied runtime object.
   *
   * @param runId externally assigned run ID
   * @param run mutable runtime state
   * @param observer optional per-generation observer
   * @return final run result
   */
  public RunResult<C> run(String runId, SearchRun<C> run, GenerationObserver<C> observer) {
    List<ScoredCandidate<C>> population = initialPopulation();
    population.sort(Comparator.comparingDouble(ScoredCandidate::score));

    ScoredCandidate<C> bestOverall = population.getFirst();
    double previousBestScore = bestOverall.score();
    int stagnationGenerations = 0;
    emit(
        run,
        observer,
        0,
        population,
        0.0,
        stagnationGenerations,
        VariationStatsBuilder.empty(),
        VariationStatsBuilder.empty());
    if (run.stopRequested()) {
      return toResult(runId, 0, bestOverall, run);
    }
    if (bestOverall.evaluation().solved()) {
      return toResult(runId, 0, bestOverall, run);
    }

    for (int generation = 1; generation <= config.maxGenerations(); generation++) {
      run.awaitIfPaused();
      if (run.stopRequested()) {
        return toResult(runId, generation - 1, bestOverall, run);
      }
      NextPopulation<C> nextPopulation = nextPopulation(population);
      population = nextPopulation.population();
      population.sort(Comparator.comparingDouble(ScoredCandidate::score));
      double bestScore = population.getFirst().score();
      double bestScoreImprovement = Math.max(0.0, previousBestScore - bestScore);
      stagnationGenerations = bestScoreImprovement > 1.0e-9 ? 0 : stagnationGenerations + 1;
      previousBestScore = Math.min(previousBestScore, bestScore);
      if (population.getFirst().score() < bestOverall.score()) {
        bestOverall = population.getFirst();
      }
      emit(
          run,
          observer,
          generation,
          population,
          bestScoreImprovement,
          stagnationGenerations,
          nextPopulation.mutationOnlyStats(),
          nextPopulation.crossoverMutationStats());
      if (run.stopRequested()) {
        return toResult(runId, generation, bestOverall, run);
      }
      if (population.getFirst().evaluation().solved()) {
        return toResult(runId, generation, population.getFirst(), run);
      }
    }

    return toResult(runId, config.maxGenerations(), bestOverall, run);
  }

  private List<ScoredCandidate<C>> initialPopulation() {
    List<ScoredCandidate<C>> population = new ArrayList<>(config.populationSize());
    for (int i = 0; i < config.populationSize(); i++) {
      population.add(score(domain.randomCandidate(rng)));
    }
    return population;
  }

  private NextPopulation<C> nextPopulation(List<ScoredCandidate<C>> current) {
    List<ScoredCandidate<C>> next = new ArrayList<>(config.populationSize());
    VariationStatsBuilder mutationOnlyStats = new VariationStatsBuilder();
    VariationStatsBuilder crossoverMutationStats = new VariationStatsBuilder();
    for (int i = 0; i < config.eliteCount(); i++) {
      next.add(current.get(i));
    }

    while (next.size() < config.populationSize()) {
      ScoredCandidate<C> parentA = tournament(current);
      C child = parentA.candidate();
      double referenceScore = parentA.score();
      VariationStatsBuilder stats = mutationOnlyStats;
      if (rng.nextDouble() < config.crossoverRate()) {
        ScoredCandidate<C> parentB = tournament(current);
        child = domain.crossover(parentA.candidate(), parentB.candidate(), rng);
        referenceScore = Math.min(parentA.score(), parentB.score());
        stats = crossoverMutationStats;
      }
      child = domain.mutate(child, rng);
      ScoredCandidate<C> scoredChild = score(child);
      next.add(scoredChild);
      stats.record(referenceScore - scoredChild.score());
    }

    return new NextPopulation<>(next, mutationOnlyStats.build(), crossoverMutationStats.build());
  }

  private ScoredCandidate<C> tournament(List<ScoredCandidate<C>> population) {
    ScoredCandidate<C> a = population.get(rng.nextInt(population.size()));
    ScoredCandidate<C> b = population.get(rng.nextInt(population.size()));
    ScoredCandidate<C> c = population.get(rng.nextInt(population.size()));
    ScoredCandidate<C> best = a.score() <= b.score() ? a : b;
    return best.score() <= c.score() ? best : c;
  }

  private ScoredCandidate<C> score(C candidate) {
    return new ScoredCandidate<>(candidate, domain.evaluate(candidate));
  }

  private void emit(SearchRun<C> run, GenerationObserver<C> observer, int generation,
                    List<ScoredCandidate<C>> population,
                    double bestScoreImprovement,
                    int stagnationGenerations,
                    VariationStats mutationOnlyStats,
                    VariationStats crossoverMutationStats) {
    ScoredCandidate<C> best = population.getFirst();
    double sum = 0.0;
    double sumSquares = 0.0;
    int solvedCount = 0;
    Set<String> uniqueFingerprints = new HashSet<>();
    Map<String, Double> metricSums = new LinkedHashMap<>();
    for (ScoredCandidate<C> entry : population) {
      sum += entry.score();
      sumSquares += entry.score() * entry.score();
      if (entry.evaluation().solved()) {
        solvedCount++;
      }
      uniqueFingerprints.add(domain.fingerprint(entry.candidate()));
      for (Map.Entry<String, Double> metric : entry.evaluation().metrics().entrySet()) {
        metricSums.merge(metric.getKey(), metric.getValue(), Double::sum);
      }
    }
    double averageScore = sum / population.size();
    double variance = (sumSquares / population.size()) - averageScore * averageScore;
    double stdDev = Math.sqrt(Math.max(0.0, variance));
    Map<String, Double> averageMetrics = new LinkedHashMap<>();
    for (Map.Entry<String, Double> metric : metricSums.entrySet()) {
      averageMetrics.put(metric.getKey(), metric.getValue() / population.size());
    }
    GenerationStats<C> stats = new GenerationStats<>(
        generation,
        best.score(),
        bestScoreImprovement,
        averageScore,
        stdDev,
        uniqueFingerprints.size() / (double) population.size(),
        stagnationGenerations,
        solvedCount,
        best.evaluation().solved(),
        averageMetrics,
        mutationOnlyStats,
        crossoverMutationStats,
        best.candidate(),
        domain.describe(best.candidate()),
        best.evaluation()
    );
    RunStatus status = best.evaluation().solved()
        ? RunStatus.SOLVED
        : (generation >= config.maxGenerations() ? RunStatus.STOPPED : RunStatus.RUNNING);
    run.addSnapshot(new RunSnapshot<>(run.runId(), status, stats));
    if (observer != null) {
      observer.onGeneration(stats);
    }
  }

  private RunResult<C> toResult(String runId, int generations, ScoredCandidate<C> best, SearchRun<C> run) {
    return new RunResult<>(
        runId,
        generations,
        best.evaluation(),
        best.candidate(),
        domain.describe(best.candidate()),
        run
    );
  }

  private record ScoredCandidate<C>(C candidate, EvaluationResult evaluation) {
    double score() {
      return evaluation.score();
    }
  }

  private record NextPopulation<C>(
      List<ScoredCandidate<C>> population,
      VariationStats mutationOnlyStats,
      VariationStats crossoverMutationStats
  ) {
  }

  private static final class VariationStatsBuilder {
    private static final double EPSILON = 1.0e-9;
    private int attempts;
    private int improvedCount;
    private int neutralCount;
    private int worsenedCount;
    private double sumDelta;
    private double bestDelta = Double.NEGATIVE_INFINITY;

    static VariationStats empty() {
      return new VariationStats(0, 0, 0, 0, 0.0, 0.0);
    }

    void record(double delta) {
      attempts++;
      sumDelta += delta;
      bestDelta = Math.max(bestDelta, delta);
      if (delta > EPSILON) {
        improvedCount++;
      } else if (delta < -EPSILON) {
        worsenedCount++;
      } else {
        neutralCount++;
      }
    }

    VariationStats build() {
      if (attempts == 0) {
        return empty();
      }
      return new VariationStats(
          attempts,
          improvedCount,
          neutralCount,
          worsenedCount,
          sumDelta / attempts,
          bestDelta);
    }
  }
}
