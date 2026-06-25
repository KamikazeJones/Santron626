package dev.quassel.ga.core;

/**
 * Observer callback for per-generation updates.
 *
 * @param <C> candidate type
 */
public interface GenerationObserver<C> {
  /**
   * Receives one generation snapshot.
   *
   * @param stats statistics of the finished generation
   */
  void onGeneration(GenerationStats<C> stats);
}
