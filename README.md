# MMORPG LFG Dungeon Queue Simulator

This C++17 console program demonstrates process synchronization through an MMORPG-style dungeon queue. It forms valid five-player parties, places them in a FIFO queue, and lets a fixed number of concurrent dungeon-instance threads serve them safely.

## Project structure

- `main.cpp` contains the small program entry point.
- `config.h` declares the configuration structure and configuration functions.
- `config.cpp` implements configuration parsing, validation, and initial output.
- `simulation.h` declares the `Simulation` class and its public/private interface.
- `simulation.cpp` implements the threaded dungeon simulation.
- `config.txt` contains the simulation inputs.
- `build.bat` and `run.bat` build and run the project on Windows.

## Assignment requirements

The simulator allows at most `n` active dungeon parties. Every complete party contains exactly one tank, one healer, and three DPS players. Dungeon runs take a random integral number of seconds from `t1` through `t2`, inclusive. The program reports live instance states, waiting parties, unmatched players, and per-instance and aggregate statistics.

All players are treated as arriving at the same time. Inputs come only from `config.txt`; the program never prompts for interactive input.

## Configuration

The six required keys are:

| Key | Meaning | Valid values |
| --- | --- | --- |
| `n` | Maximum concurrent dungeon instances | Greater than 0 |
| `t` | Tank players | 0 or greater |
| `h` | Healer players | 0 or greater |
| `d` | DPS players | 0 or greater |
| `t1` | Minimum dungeon duration in seconds | `0 <= t1` |
| `t2` | Maximum dungeon duration in seconds | `t1 <= t2 <= 15` |

Edit `config.txt` before running the program. Entries may be in any order and may have whitespace around keys and values. Blank lines and lines whose first non-whitespace character is `#` are ignored.

```ini
n=2
t=5
h=5
d=15
t1=3
t2=8
```

Missing keys, duplicate or unknown keys, malformed integers, and out-of-range values produce a descriptive error and a nonzero exit code. No worker threads are started after a configuration error.

## Party formation

The number of parties is calculated as:

```cpp
std::min({t, h, d / 3})
```

Parties receive sequential IDs and are inserted into a FIFO queue. Each party consumes one tank, one healer, and three DPS players. Remaining players are reported by role; incomplete parties are never queued.

## Concurrency and synchronization

The program creates `min(n, number of parties)` worker threads—one for each dungeon instance that can initially be used. All `n` instances still appear in status output and the final summary, so excess instances remain visible and empty.

Each worker repeatedly removes the next party from the shared queue, marks its own instance active, sleeps for that party's random dungeon duration, records completion, marks the instance empty, and continues until the queue is empty. The main thread joins every worker before printing the final summary.

A single shared mutex protects all mutable shared state:

- The FIFO party queue
- Instance status and statistics
- Coordinated console events and full status snapshots

Queue removal and assignment happen in the same critical section, so a party cannot be assigned twice and an instance cannot hold two parties. Completion counters and status transitions are also protected. Each complete console snapshot is printed while holding the mutex, preventing interleaved output.

The mutex is always acquired through `std::lock_guard` and there is only one mutex, eliminating circular lock ordering. Crucially, a worker releases the mutex before `sleep_for`, so a running dungeon never blocks another instance from obtaining work. The code contains no busy-waiting. FIFO removal gives every party a fixed place in the finite queue, so every formed party is eventually served and starvation is avoided.

## Build and run

Run commands from the directory containing `main.cpp` and `config.txt`.

Compile with g++:

```bash
g++ -std=c++17 -pthread main.cpp config.cpp simulation.cpp -o dungeon_lfg
```

On Linux or macOS:

```bash
./dungeon_lfg
```

On Windows PowerShell (MinGW-w64 g++):

```powershell
.\dungeon_lfg.exe
```

On this Windows project, you can also build with the included Visual Studio batch file:

```powershell
.\build.bat
```

Then run the compiled simulator separately:

```powershell
.\run.bat
```

Because the program looks for `config.txt` in its current working directory, start it from the project directory.

## Reading the output

At startup, the program displays the loaded values, complete parties, unmatched players by role, and initial waiting count. Whenever a party enters or finishes, it prints an elapsed timestamp and a synchronized snapshot of every instance. An occupied instance is `active (Party N)`; an unoccupied one is `empty`.

The final summary reports every instance's final state, party count, and cumulative service time. **Total Instance Service Time** is the sum of all simulated dungeon durations. **Total Simulation Time** is elapsed wall-clock time. Because instances run concurrently, cumulative service time may be much larger than wall-clock time.
