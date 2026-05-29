# AFL++ Fuzzing Setup

This project uses AFL++ for fuzzing the MisraStdC library. We provide both Docker-based and CI fuzzing capabilities.

## Quick Start

### Local Fuzzing with Docker

1. **Build and run fuzzing:**
   ```bash
   ./scripts/fuzz-docker.sh
   ```

2. **Fuzzing with AddressSanitizer:**
   - Always uses AFL++ with AddressSanitizer for comprehensive bug detection
   - Catches memory bugs (use-after-free, buffer overflows, etc.)

### Manual Docker Commands

If you prefer to run Docker commands manually:

```bash
# Build the fuzzing image
docker build -f Dockerfile.fuzz -t misra-fuzz .

# Run AFL++ with ASAN (only option)
docker run -it --rm \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "/usr/local/bin/fuzz.sh"
```

## Fuzzing Approach

### AFL++ with AddressSanitizer
- **Pros**: Catches memory bugs (use-after-free, buffer overflows, etc.)
- **Cons**: Slower execution, higher memory usage
- **Use case**: Comprehensive testing and bug hunting

## Fuzzing Results

Results are saved in the `fuzz-outputs/` directory:
- `fuzzer-asan/queue/` - Interesting test cases found
- `fuzzer-asan/crashes/` - Crashes discovered
- `fuzzer-asan/hangs/` - Timeout cases

## CI/CD Integration

The project includes GitHub Actions workflow (`.github/workflows/fuzz.yml`) that automatically runs fuzzing using the Docker-based setup.

### GitHub CI Fuzzing

The CI workflow is triggered by:
- **Push** to `main` or `develop` branches
- **Pull Requests** to `main` branch
- **Daily schedule** at 2 AM UTC
- **Manual trigger** via workflow_dispatch

### CI Process

1. **Builds Docker image** using `Dockerfile.fuzz`
2. **Runs AFL++ with ASAN** (30 min timeout)
3. **Checks for crashes** in fuzzing results
4. **Uploads artifacts** with fuzzing results
5. **Comments on PRs** with results summary

### CI Features

- ✅ **Uses `fuzz-docker.sh` script directly**
- ✅ **ASAN fuzzing enabled by default**
- ✅ **30-minute timeout per fuzzing run**
- ✅ **Crash detection and reporting**
- ✅ **Artifact upload for crash analysis**
- ✅ **PR comments with results**

### Crash Detection

The CI will:
- **Fail the build** if crashes are found
- **Upload crash files** as artifacts
- **Comment on PRs** with crash status
- **Provide crash details** for debugging

### Artifacts

All fuzzing results are saved as GitHub artifacts:
- **Crashes** (if any found)
- **Hangs** (timeout cases)
- **Queue** (interesting test cases)
- **Statistics** (coverage data)

### CI Usage

The CI automatically runs when you:
- Push code to `main` or `develop`
- Create a Pull Request
- Manually trigger the workflow

No additional setup required - the CI uses the same Docker setup as local fuzzing.

## Fuzzing Harness

The fuzzing harness (`Fuzz/Harness.c`) tests:
- **Vec(i32)** - Integer vector operations (`Fuzz/Harness/VecInt.c`)
- **Vec(char*)** - C-string pointer vector operations (`Fuzz/Harness/VecCharPtr.c`)
- **Vec(Str)** - String vector operations (`Fuzz/Harness/VecStr.c`)
- **List(i32)** - Integer doubly-linked-list operations (`Fuzz/Harness/ListInt.c`)

Each sub-harness covers 30+ functions per container with comprehensive test cases.

## Troubleshooting

### Docker Issues
- Ensure Docker is running: `docker info`
- Check available disk space for fuzzing outputs
- Increase Docker memory limits if needed

### Fuzzing Issues
- Check crash logs in `fuzz-outputs/default/crashes/`
- Verify input format (first 2 bytes: object type, next 2 bytes: function)
- Ensure sufficient memory for ASAN builds

### Performance
- AFL++ with ASAN: ~100-500 execs/sec
- Adjust timeout values based on your needs

## Advanced Usage

### Custom Fuzzing Parameters
```bash
# Run with custom timeout and memory limit
docker run -it --rm \
  -e AFL_TIMEOUT=5000 \
  -e AFL_MEM_LIMIT=200 \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "/usr/local/bin/build_afl_asan.sh && cd build-afl-asan && afl-fuzz -t 5000 -m 200 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness"
```

### Parallel Fuzzing
```bash
# Run multiple fuzzer instances
docker run -it --rm \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "
    /usr/local/bin/build_afl_asan.sh
    cd build-afl-asan
    afl-fuzz -M fuzzer1 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness &
    afl-fuzz -S fuzzer2 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness &
    wait
  "
```

## Contributing

When adding new functions to the library:
1. Add corresponding test cases to the appropriate harness
2. Update the function enumeration in the harness header
3. Test with AFL++ with ASAN
4. Ensure no crashes in CI fuzzing
