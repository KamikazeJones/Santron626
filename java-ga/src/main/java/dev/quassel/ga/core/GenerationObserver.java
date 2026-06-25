package dev.quassel.ga.core;

public interface GenerationObserver<C> {
  void onGeneration(GenerationStats<C> stats);
}
