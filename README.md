# search_recorder
*Search Recorder* uses the same protocol as the profiler, but is designed to record coming from a solver messages (nodes and commands) to a file.
The resulting file is readable by [*Search Reader*](https://github.com/cp-profiler/search_reader).
The file can then be read by the search_reader to simulate the original solver execution.
