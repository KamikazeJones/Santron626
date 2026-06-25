package dev.quassel.ga.core;

import java.util.random.RandomGenerator;

/**
 * Domain contract for the generic search engine.
 *
 * <p>The GA core knows nothing about a concrete problem. A domain provides
 * candidate creation, variation, evaluation and a stable fingerprint for
 * diversity and duplicate detection.
 *
 * @param <C> candidate type of the concrete domain
 */
public interface SearchDomain<C> {
  /**
   * Creates a fresh random candidate.
   *
   * @param rng random source controlled by the GA
   * @return a newly created candidate
   */
  C randomCandidate(RandomGenerator rng);

  /**
   * Produces a mutated candidate from an existing one.
   *
   * @param candidate source candidate
   * @param rng random source controlled by the GA
   * @return mutated candidate
   */
  C mutate(C candidate, RandomGenerator rng);

  /**
   * Combines two parents into a child candidate.
   *
   * <p>The default implementation simply returns one of the parents.
   *
   * @param left first parent
   * @param right second parent
   * @param rng random source controlled by the GA
   * @return child candidate
   */
  default C crossover(C left, C right, RandomGenerator rng) {
    return rng.nextBoolean() ? left : right;
  }

  /**
   * Evaluates a candidate and returns a structured result.
   *
   * @param candidate candidate to evaluate
   * @return structured evaluation
   */
  EvaluationResult evaluate(C candidate);

  /**
   * Returns a human-readable description of a candidate.
   *
   * @param candidate candidate to describe
   * @return textual description
   */
  String describe(C candidate);

  /**
   * Returns a stable fingerprint used for duplicate detection and diversity
   * estimates.
   *
   * <p>Implementations should prefer a canonical problem-specific identity
   * instead of a verbose textual description whenever possible.
   *
   * @param candidate candidate to fingerprint
   * @return stable identity string for duplicate detection
   */
  default String fingerprint(C candidate) {
    return describe(candidate);
  }
}
