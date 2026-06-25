package dev.quassel.ga.core;

import java.util.random.RandomGenerator;

public interface SearchDomain<C> {
  C randomCandidate(RandomGenerator rng);

  C mutate(C candidate, RandomGenerator rng);

  default C crossover(C left, C right, RandomGenerator rng) {
    return rng.nextBoolean() ? left : right;
  }

  EvaluationResult evaluate(C candidate);

  String describe(C candidate);
}
