package dev.quassel.ga.puzzle;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public final class PuzzleBoard {
  private final int size;
  private final int[] tiles;
  private final int blankIndex;

  public PuzzleBoard(int size, int[] tiles) {
    if (size < 2) {
      throw new IllegalArgumentException("size must be >= 2");
    }
    if (tiles.length != size * size) {
      throw new IllegalArgumentException("unexpected tile count");
    }
    this.size = size;
    this.tiles = Arrays.copyOf(tiles, tiles.length);
    this.blankIndex = findBlank(this.tiles);
  }

  public static PuzzleBoard solved(int size) {
    int[] tiles = new int[size * size];
    for (int i = 0; i < tiles.length - 1; i++) {
      tiles[i] = i + 1;
    }
    tiles[tiles.length - 1] = 0;
    return new PuzzleBoard(size, tiles);
  }

  public static PuzzleBoard parse(String text, int size) {
    String normalized = text.trim().replace('/', ' ').replace(',', ' ');
    String[] parts = normalized.split("\\s+");
    int expectedCount = size * size;
    if (parts.length != expectedCount) {
      throw new IllegalArgumentException("expected " + expectedCount + " tile values for a "
          + size + "x" + size + " board");
    }
    int[] tiles = new int[expectedCount];
    Set<Integer> seen = new HashSet<>();
    for (int i = 0; i < parts.length; i++) {
      String part = parts[i];
      if ("_".equals(part) || ".".equals(part)) {
        part = "0";
      }
      int value;
      try {
        value = Integer.parseInt(part);
      } catch (NumberFormatException ex) {
        throw new IllegalArgumentException("invalid tile value: " + parts[i], ex);
      }
      if (value < 0 || value >= expectedCount) {
        throw new IllegalArgumentException("tile out of range: " + value);
      }
      if (!seen.add(value)) {
        throw new IllegalArgumentException("duplicate tile value: " + value);
      }
      tiles[i] = value;
    }
    if (seen.size() != expectedCount) {
      throw new IllegalArgumentException(
          "board must contain every tile from 0 to " + (expectedCount - 1) + " exactly once");
    }
    return new PuzzleBoard(size, tiles);
  }

  public int size() {
    return size;
  }

  public PuzzleBoard apply(Move move) {
    int row = blankIndex / size;
    int col = blankIndex % size;
    int nextRow = row + move.rowDelta;
    int nextCol = col + move.colDelta;
    if (nextRow < 0 || nextRow >= size || nextCol < 0 || nextCol >= size) {
      return this;
    }
    int swapIndex = nextRow * size + nextCol;
    int[] copy = Arrays.copyOf(tiles, tiles.length);
    copy[blankIndex] = copy[swapIndex];
    copy[swapIndex] = 0;
    return new PuzzleBoard(size, copy);
  }

  public PuzzleBoard apply(Iterable<Move> moves) {
    PuzzleBoard board = this;
    for (Move move : moves) {
      board = board.apply(move);
    }
    return board;
  }

  public ExecutionResult execute(Iterable<Move> moves) {
    PuzzleBoard board = this;
    int effectiveMoves = 0;
    int ineffectiveMoves = 0;
    for (Move move : moves) {
      PuzzleBoard next = board.apply(move);
      if (next.equals(board)) {
        ineffectiveMoves++;
      } else {
        effectiveMoves++;
      }
      board = next;
    }
    return new ExecutionResult(board, effectiveMoves, ineffectiveMoves);
  }

  public boolean isSolved() {
    for (int i = 0; i < tiles.length - 1; i++) {
      if (tiles[i] != i + 1) {
        return false;
      }
    }
    return tiles[tiles.length - 1] == 0;
  }

  public int manhattanDistance() {
    int distance = 0;
    for (int index = 0; index < tiles.length; index++) {
      int value = tiles[index];
      if (value == 0) {
        continue;
      }
      int expectedIndex = value - 1;
      int row = index / size;
      int col = index % size;
      int expectedRow = expectedIndex / size;
      int expectedCol = expectedIndex % size;
      distance += Math.abs(row - expectedRow) + Math.abs(col - expectedCol);
    }
    return distance;
  }

  public int misplacedTiles() {
    int misplaced = 0;
    for (int i = 0; i < tiles.length - 1; i++) {
      if (tiles[i] != i + 1) {
        misplaced++;
      }
    }
    return misplaced;
  }

  public boolean isSolvable() {
    int inversions = inversionCount();
    if ((size & 1) == 1) {
      return (inversions & 1) == 0;
    }
    int blankRowFromBottom = size - (blankIndex / size);
    return ((inversions + blankRowFromBottom) & 1) == 1;
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder();
    for (int row = 0; row < size; row++) {
      for (int col = 0; col < size; col++) {
        int value = tiles[row * size + col];
        if (col > 0) {
          sb.append(' ');
        }
        sb.append(String.format("%2s", value == 0 ? "_" : Integer.toString(value)));
      }
      if (row + 1 < size) {
        sb.append(System.lineSeparator());
      }
    }
    return sb.toString();
  }

  public static void selfTest() {
    PuzzleBoard solved = PuzzleBoard.solved(5);
    if (!solved.isSolved()) {
      throw new IllegalStateException("solved board should be solved");
    }
    if (!solved.isSolvable()) {
      throw new IllegalStateException("solved board should be solvable");
    }
    PuzzleBoard moved = solved.apply(Move.L);
    if (moved == solved || moved.isSolved()) {
      throw new IllegalStateException("left move should change solved board");
    }
    PuzzleBoard restored = moved.apply(Move.R);
    if (!restored.equals(solved)) {
      throw new IllegalStateException("inverse move should restore board");
    }
    if (moved.manhattanDistance() <= 0) {
      throw new IllegalStateException("moved board should have positive distance");
    }
    PuzzleBoard parsed = PuzzleBoard.parse(
        "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 0", 5);
    if (!parsed.equals(solved)) {
      throw new IllegalStateException("parsed board should match solved board");
    }
    PuzzleBoard unsolvable = PuzzleBoard.parse(
        "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 24 23 0", 5);
    if (unsolvable.isSolvable()) {
      throw new IllegalStateException("swapped 23/24 board should be unsolvable");
    }
  }

  @Override
  public boolean equals(Object obj) {
    if (!(obj instanceof PuzzleBoard other)) {
      return false;
    }
    return size == other.size && Arrays.equals(tiles, other.tiles);
  }

  @Override
  public int hashCode() {
    return 31 * size + Arrays.hashCode(tiles);
  }

  private static int findBlank(int[] tiles) {
    for (int i = 0; i < tiles.length; i++) {
      if (tiles[i] == 0) {
        return i;
      }
    }
    throw new IllegalArgumentException("board must contain blank tile 0");
  }

  private int inversionCount() {
    int inversions = 0;
    for (int i = 0; i < tiles.length; i++) {
      if (tiles[i] == 0) {
        continue;
      }
      for (int j = i + 1; j < tiles.length; j++) {
        if (tiles[j] == 0) {
          continue;
        }
        if (tiles[i] > tiles[j]) {
          inversions++;
        }
      }
    }
    return inversions;
  }

  public record ExecutionResult(PuzzleBoard board, int effectiveMoves, int ineffectiveMoves) {
  }
}
