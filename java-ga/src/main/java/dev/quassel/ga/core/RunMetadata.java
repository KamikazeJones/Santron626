package dev.quassel.ga.core;

import java.util.Map;

/**
 * Static metadata known at run creation time.
 *
 * @param request normalized problem request parameters
 * @param config normalized GA configuration
 */
public record RunMetadata(
    Map<String, String> request,
    Map<String, String> config
) {
}
