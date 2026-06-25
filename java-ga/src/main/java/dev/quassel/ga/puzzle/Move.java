package dev.quassel.ga.puzzle;

/** Primitive puzzle moves of the blank tile. */
public enum Move {
  /** Move the blank up. */
  U(-1, 0),
  /** Move the blank down. */
  D(1, 0),
  /** Move the blank left. */
  L(0, -1),
  /** Move the blank right. */
  R(0, 1);

  final int rowDelta;
  final int colDelta;

  Move(int rowDelta, int colDelta) {
    this.rowDelta = rowDelta;
    this.colDelta = colDelta;
  }
}
