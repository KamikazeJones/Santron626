package dev.quassel.ga.puzzle;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Move-sequence candidate for the sliding-puzzle domain. */
public final class PuzzleCandidate {
  private final List<Move> moves;

  /**
   * Creates a candidate from a move list.
   *
   * @param moves move sequence
   */
  public PuzzleCandidate(List<Move> moves) {
    this.moves = List.copyOf(moves);
  }

  /**
   * Returns the immutable move sequence.
   *
   * @return move sequence
   */
  public List<Move> moves() {
    return moves;
  }

  /**
   * Returns a new candidate with a replaced move sequence.
   *
   * @param newMoves replacement move sequence
   * @return new candidate
   */
  public PuzzleCandidate withMoves(List<Move> newMoves) {
    return new PuzzleCandidate(newMoves);
  }

  /**
   * Compact one-letter move encoding.
   *
   * @return compact move string
   */
  public String compact() {
    StringBuilder sb = new StringBuilder(moves.size());
    for (Move move : moves) {
      sb.append(move.name());
    }
    return sb.toString();
  }

  @Override
  public String toString() {
    return compact();
  }

  /**
   * Empty candidate without moves.
   *
   * @return empty candidate
   */
  public static PuzzleCandidate empty() {
    return new PuzzleCandidate(Collections.emptyList());
  }

  /**
   * Utility helper to copy a move list and replace one entry.
   *
   * @param source source move list
   * @param index index to replace
   * @param value new move value
   * @return new candidate
   */
  public static PuzzleCandidate copyAndReplace(List<Move> source, int index, Move value) {
    List<Move> copy = new ArrayList<>(source);
    copy.set(index, value);
    return new PuzzleCandidate(copy);
  }
}
