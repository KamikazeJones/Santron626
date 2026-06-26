package dev.quassel.ga.core;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Mutable runtime state of one concrete search run.
 *
 * <p>This object is owned by the runtime/registry layer. It stores snapshots,
 * control flags and final result information for external inspection.
 *
 * @param <C> candidate type
 */
public final class SearchRun<C> {
  private final String runId;
  private final List<RunSnapshot<C>> snapshots = new ArrayList<>();
  private final long startedAtMillis = System.currentTimeMillis();
  private RunStatus status = RunStatus.RUNNING;
  private RunMetadata metadata;
  private RunResult<C> finalResult;
  private Throwable failure;
  private boolean pauseRequested;
  private boolean stopRequested;

  /**
   * Creates a runtime object for one run ID.
   *
   * @param runId unique run identifier
   */
  public SearchRun(String runId) {
    this.runId = runId;
  }

  /**
   * Returns the unique run identifier.
   *
   * @return run ID
   */
  public String runId() {
    return runId;
  }

  /**
   * Returns the current lifecycle state of the run.
   *
   * @return current status
   */
  public synchronized RunStatus status() {
    return status;
  }

  /**
   * Returns the wall-clock start time in milliseconds since the epoch.
   *
   * @return start timestamp
   */
  public long startedAtMillis() {
    return startedAtMillis;
  }

  /**
   * Returns an immutable view of all collected snapshots.
   *
   * @return snapshot list
   */
  public synchronized List<RunSnapshot<C>> snapshots() {
    return Collections.unmodifiableList(snapshots);
  }

  /**
   * Returns the most recent snapshot, or {@code null} if the run has not
   * emitted one yet.
   *
   * @return latest snapshot or {@code null}
   */
  public synchronized RunSnapshot<C> latestSnapshot() {
    return snapshots.isEmpty() ? null : snapshots.getLast();
  }

  /**
   * Returns the final result once the run has finished.
   *
   * @return final result or {@code null}
   */
  public synchronized RunResult<C> finalResult() {
    return finalResult;
  }

  /**
   * Returns static metadata captured for this run.
   *
   * @return run metadata or {@code null}
   */
  public synchronized RunMetadata metadata() {
    return metadata;
  }

  /**
   * Returns the terminal failure if the run crashed.
   *
   * @return failure cause or {@code null}
   */
  public synchronized Throwable failure() {
    return failure;
  }

  /**
   * Whether a pause has been requested.
   *
   * @return true if pause is requested
   */
  public synchronized boolean pauseRequested() {
    return pauseRequested;
  }

  /**
   * Whether a stop has been requested.
   *
   * @return true if stop is requested
   */
  public synchronized boolean stopRequested() {
    return stopRequested;
  }

  /** Requests the run to pause at the next safe synchronization point. */
  public synchronized void requestPause() {
    if (status == RunStatus.RUNNING) {
      pauseRequested = true;
      status = RunStatus.PAUSED;
      notifyAll();
    }
  }

  /** Resumes a previously paused run. */
  public synchronized void requestResume() {
    pauseRequested = false;
    if (status == RunStatus.PAUSED) {
      status = RunStatus.RUNNING;
    }
    notifyAll();
  }

  /** Requests the run to stop at the next safe synchronization point. */
  public synchronized void requestStop() {
    stopRequested = true;
    pauseRequested = false;
    status = RunStatus.STOPPED;
    notifyAll();
  }

  /** Blocks the calling worker while the run is paused. */
  public synchronized void awaitIfPaused() {
    while (pauseRequested && !stopRequested) {
      try {
        wait();
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
        stopRequested = true;
        status = RunStatus.STOPPED;
        break;
      }
    }
  }

  synchronized void addSnapshot(RunSnapshot<C> snapshot) {
    RunStatus snapshotStatus = stopRequested
        ? RunStatus.STOPPED
        : (pauseRequested && snapshot.status() == RunStatus.RUNNING ? RunStatus.PAUSED : snapshot.status());
    snapshots.add(new RunSnapshot<>(snapshot.runId(), snapshotStatus, snapshot.generationStats()));
    status = snapshotStatus;
  }

  synchronized void setFinalResult(RunResult<C> finalResult) {
    this.finalResult = finalResult;
  }

  /** Stores static metadata for this run. */
  public synchronized void setMetadata(RunMetadata metadata) {
    this.metadata = metadata;
  }

  synchronized void markStopped() {
    status = RunStatus.STOPPED;
  }

  synchronized void setFailure(Throwable failure) {
    this.failure = failure;
  }
}
