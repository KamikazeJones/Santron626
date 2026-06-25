package dev.quassel.ga.core;

/**
 * Lifecycle states of a search run.
 */
public enum RunStatus {
  /** Run is currently advancing. */
  RUNNING,
  /** Run is temporarily paused and can later continue. */
  PAUSED,
  /** Run found a solved candidate. */
  SOLVED,
  /** Run was stopped or ended without solving. */
  STOPPED
}
