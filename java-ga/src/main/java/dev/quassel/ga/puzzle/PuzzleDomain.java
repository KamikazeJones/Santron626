package dev.quassel.ga.puzzle;

import dev.quassel.ga.core.EvaluationResult;
import dev.quassel.ga.core.SearchDomain;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.ArrayDeque;
import java.util.random.RandomGenerator;

public final class PuzzleDomain implements SearchDomain<PuzzleCandidate> {
  private final PuzzleBoard startBoard;
  private final int minLength;
  private final int maxLength;

  public PuzzleDomain(PuzzleBoard startBoard, int minLength, int maxLength) {
    if (minLength < 0 || maxLength < minLength) {
      throw new IllegalArgumentException("invalid length range");
    }
    this.startBoard = startBoard;
    this.minLength = minLength;
    this.maxLength = maxLength;
  }

  @Override
  public PuzzleCandidate randomCandidate(RandomGenerator rng) {
    int length = rng.nextInt(minLength, maxLength + 1);
    List<Move> moves = new ArrayList<>(length);
    for (int i = 0; i < length; i++) {
      moves.add(randomMove(rng));
    }
    return normalizedCandidate(moves);
  }

  @Override
  public PuzzleCandidate mutate(PuzzleCandidate candidate, RandomGenerator rng) {
    List<Move> moves = new ArrayList<>(candidate.moves());
    int mode = rng.nextInt(4);
    if (moves.isEmpty()) {
      moves.add(randomMove(rng));
      return new PuzzleCandidate(moves);
    }
    switch (mode) {
      case 0 -> {
        int index = rng.nextInt(moves.size());
        moves.set(index, randomMove(rng));
      }
      case 1 -> {
        if (moves.size() < maxLength) {
          int index = rng.nextInt(moves.size() + 1);
          moves.add(index, randomMove(rng));
        } else {
          int index = rng.nextInt(moves.size());
          moves.set(index, randomMove(rng));
        }
      }
      case 2 -> {
        if (moves.size() > minLength) {
          int index = rng.nextInt(moves.size());
          moves.remove(index);
        } else {
          int index = rng.nextInt(moves.size());
          moves.set(index, randomMove(rng));
        }
      }
      default -> {
        if (moves.size() >= 2) {
          int a = rng.nextInt(moves.size());
          int b = rng.nextInt(moves.size());
          Move tmp = moves.get(a);
          moves.set(a, moves.get(b));
          moves.set(b, tmp);
        } else {
          moves.set(0, randomMove(rng));
        }
      }
    }
    return normalizedCandidate(moves);
  }

  @Override
  public PuzzleCandidate crossover(PuzzleCandidate left, PuzzleCandidate right, RandomGenerator rng) {
    if (left.moves().isEmpty()) {
      return right;
    }
    if (right.moves().isEmpty()) {
      return left;
    }
    int splitA = rng.nextInt(left.moves().size() + 1);
    int splitB = rng.nextInt(right.moves().size() + 1);
    List<Move> merged = new ArrayList<>(left.moves().subList(0, splitA));
    merged.addAll(right.moves().subList(splitB, right.moves().size()));
    if (merged.size() > maxLength) {
      merged = new ArrayList<>(merged.subList(0, maxLength));
    }
    while (merged.size() < minLength) {
      merged.add(randomMove(rng));
    }
    return normalizedCandidate(merged);
  }

  @Override
  public EvaluationResult evaluate(PuzzleCandidate candidate) {
    PuzzleCandidate normalized = normalizedCandidate(candidate.moves());
    PuzzleBoard.ExecutionResult execution = startBoard.execute(normalized.moves());
    PuzzleBoard end = execution.board();
    int manhattan = end.manhattanDistance();
    int misplaced = end.misplacedTiles();
    boolean solved = end.isSolved();
    int effectiveMoves = execution.effectiveMoves();
    int ineffectiveMoves = execution.ineffectiveMoves();
    int rawLength = candidate.moves().size();
    int normalizedLength = normalized.moves().size();
    double lengthPenalty = normalizedLength * 0.05;
    double ineffectivePenalty = ineffectiveMoves * 2.5;
    double normalizationPenalty = Math.max(0, rawLength - normalizedLength) * 0.5;
    double score = manhattan * 10.0 + misplaced + lengthPenalty + ineffectivePenalty + normalizationPenalty;
    if (solved) {
      score = normalizedLength + ineffectiveMoves * 0.1;
    }
    String summary = "moves=" + normalized.compact()
        + System.lineSeparator()
        + "end-distance=" + manhattan
        + " misplaced=" + misplaced
        + " raw-length=" + rawLength
        + " normalized-length=" + normalizedLength
        + " effective=" + effectiveMoves
        + " ineffective=" + ineffectiveMoves
        + System.lineSeparator()
        + end;
    return new EvaluationResult(score, solved, summary);
  }

  @Override
  public String describe(PuzzleCandidate candidate) {
    PuzzleCandidate normalized = normalizedCandidate(candidate.moves());
    EvaluationResult result = evaluate(candidate);
    return "score=" + result.score()
        + " solved=" + result.solved()
        + " len=" + normalized.moves().size()
        + System.lineSeparator()
        + result.summary();
  }

  public static PuzzleBoard scrambledBoard(int size, RandomGenerator rng) {
    PuzzleBoard board = PuzzleBoard.solved(size);
    int shuffleCount = 2 * size * size;
    for (int i = 0; i < shuffleCount; i++) {
      board = board.apply(randomMove(rng));
    }
    return board;
  }

  private static Move randomMove(RandomGenerator rng) {
    Move[] moves = Move.values();
    return moves[rng.nextInt(moves.length)];
  }

  private PuzzleCandidate normalizedCandidate(List<Move> moves) {
    Deque<Move> stack = new ArrayDeque<>();
    for (Move move : moves) {
      Move previous = stack.peekLast();
      if (previous != null && areInverse(previous, move)) {
        stack.removeLast();
      } else {
        stack.addLast(move);
      }
    }

    List<Move> normalized = new ArrayList<>(stack);
    while (normalized.size() > maxLength) {
      normalized.remove(normalized.size() - 1);
    }
    while (normalized.size() < minLength) {
      normalized.add(Move.U);
    }
    return new PuzzleCandidate(normalized);
  }

  private static boolean areInverse(Move a, Move b) {
    return (a == Move.U && b == Move.D)
        || (a == Move.D && b == Move.U)
        || (a == Move.L && b == Move.R)
        || (a == Move.R && b == Move.L);
  }
}
