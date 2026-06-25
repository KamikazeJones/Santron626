package dev.quassel.ga.puzzle;

public enum Move {
  U(-1, 0),
  D(1, 0),
  L(0, -1),
  R(0, 1);

  final int rowDelta;
  final int colDelta;

  Move(int rowDelta, int colDelta) {
    this.rowDelta = rowDelta;
    this.colDelta = colDelta;
  }
}
