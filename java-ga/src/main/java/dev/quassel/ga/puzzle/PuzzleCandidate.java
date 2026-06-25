package dev.quassel.ga.puzzle;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class PuzzleCandidate {
  private final List<Move> moves;

  public PuzzleCandidate(List<Move> moves) {
    this.moves = List.copyOf(moves);
  }

  public List<Move> moves() {
    return moves;
  }

  public PuzzleCandidate withMoves(List<Move> newMoves) {
    return new PuzzleCandidate(newMoves);
  }

  public String compact() {
    StringBuilder sb = new StringBuilder(moves.size());
    for (Move move : moves) {
      sb.append(move.name());
    }
    return sb.toString();
  }

  public static PuzzleCandidate empty() {
    return new PuzzleCandidate(Collections.emptyList());
  }

  public static PuzzleCandidate copyAndReplace(List<Move> source, int index, Move value) {
    List<Move> copy = new ArrayList<>(source);
    copy.set(index, value);
    return new PuzzleCandidate(copy);
  }
}
