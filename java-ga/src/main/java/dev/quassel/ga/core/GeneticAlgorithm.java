package dev.quassel.ga.core;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

public final class GeneticAlgorithm<C> {
  private final SearchDomain<C> domain;
  private final GeneticAlgorithmConfig config;
  private final RandomGenerator rng;

  public GeneticAlgorithm(SearchDomain<C> domain, GeneticAlgorithmConfig config) {
    this.domain = domain;
    this.config = config;
    this.rng = RandomGeneratorFactory.getDefault().create(config.seed());
  }

  public RunResult<C> run(GenerationObserver<C> observer) {
    List<ScoredCandidate<C>> population = initialPopulation();
    population.sort(Comparator.comparingDouble(ScoredCandidate::score));

    ScoredCandidate<C> bestOverall = population.getFirst();
    emit(observer, 0, population);
    if (bestOverall.evaluation().solved()) {
      return toResult(0, bestOverall);
    }

    for (int generation = 1; generation <= config.maxGenerations(); generation++) {
      population = nextPopulation(population);
      population.sort(Comparator.comparingDouble(ScoredCandidate::score));
      if (population.getFirst().score() < bestOverall.score()) {
        bestOverall = population.getFirst();
      }
      emit(observer, generation, population);
      if (population.getFirst().evaluation().solved()) {
        return toResult(generation, population.getFirst());
      }
    }

    return toResult(config.maxGenerations(), bestOverall);
  }

  private List<ScoredCandidate<C>> initialPopulation() {
    List<ScoredCandidate<C>> population = new ArrayList<>(config.populationSize());
    for (int i = 0; i < config.populationSize(); i++) {
      population.add(score(domain.randomCandidate(rng)));
    }
    return population;
  }

  private List<ScoredCandidate<C>> nextPopulation(List<ScoredCandidate<C>> current) {
    List<ScoredCandidate<C>> next = new ArrayList<>(config.populationSize());
    for (int i = 0; i < config.eliteCount(); i++) {
      next.add(current.get(i));
    }

    while (next.size() < config.populationSize()) {
      ScoredCandidate<C> parentA = tournament(current);
      C child = parentA.candidate();
      if (rng.nextDouble() < config.crossoverRate()) {
        ScoredCandidate<C> parentB = tournament(current);
        child = domain.crossover(parentA.candidate(), parentB.candidate(), rng);
      }
      child = domain.mutate(child, rng);
      next.add(score(child));
    }

    return next;
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

  private void emit(GenerationObserver<C> observer, int generation, List<ScoredCandidate<C>> population) {
    if (observer == null) {
      return;
    }
    ScoredCandidate<C> best = population.getFirst();
    double sum = 0.0;
    for (ScoredCandidate<C> entry : population) {
      sum += entry.score();
    }
    observer.onGeneration(new GenerationStats<>(
        generation,
        best.score(),
        sum / population.size(),
        best.evaluation().solved(),
        best.candidate(),
        domain.describe(best.candidate())
    ));
  }

  private RunResult<C> toResult(int generations, ScoredCandidate<C> best) {
    return new RunResult<>(generations, best.evaluation(), best.candidate(), domain.describe(best.candidate()));
  }

  private record ScoredCandidate<C>(C candidate, EvaluationResult evaluation) {
    double score() {
      return evaluation.score();
    }
  }
}
