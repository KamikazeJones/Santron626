package dev.quassel.ga.tsp;

import java.util.List;

/** One round-trip candidate represented as a permutation of city indices. */
public final class TspCandidate {
  private final List<Integer> route;

  /**
   * Creates a route candidate from a permutation of city indices.
   *
   * @param route city visit order
   */
  public TspCandidate(List<Integer> route) {
    this.route = List.copyOf(route);
  }

  /**
   * Immutable city visit order.
   *
   * @return route permutation
   */
  public List<Integer> route() {
    return route;
  }

  @Override
  public String toString() {
    return route.toString();
  }
}
