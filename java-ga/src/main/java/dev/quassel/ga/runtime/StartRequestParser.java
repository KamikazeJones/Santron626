package dev.quassel.ga.runtime;

import java.util.Map;

/**
 * Parses transport-level parameters into a typed run request.
 *
 * @param <R> request/specification type
 */
public interface StartRequestParser<R> {
  /**
   * Parses one request from query parameters.
   *
   * @param params decoded query parameters
   * @return typed request
   */
  R parseRequest(Map<String, String> params);
}
