# AFL++ Fuzzing Setup

This project uses AFL++ for fuzzing the MisraStdC library. We provide both Docker-based and CI fuzzing capabilities.

## Quick Start

### Local Fuzzing with Docker

1. **Build and run fuzzing:**
   ```bash
   ./scripts/fuzz-docker.sh
   ```

2. **Choose your fuzzing approach:**
   - **Option 1**: AFL++ without AddressSanitizer (faster, basic coverage)
   - **Option 2**: AFL++ with AddressSanitizer (slower, better bug detection)
   - **Option 3**: Build both and choose later

### Manual Docker Commands

If you prefer to run Docker commands manually:

```bash
# Build the fuzzing image
docker build -f Dockerfile.fuzz -t misra-fuzz .

# Run AFL++ without ASAN
docker run -it --rm \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "/usr/local/bin/build_afl.sh && /usr/local/bin/fuzz.sh no-asan"

# Run AFL++ with ASAN
docker run -it --rm \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "/usr/local/bin/build_afl_asan.sh && /usr/local/bin/fuzz.sh asan"
```

## Fuzzing Approaches

### 1. AFL++ without AddressSanitizer
- **Pros**: Faster execution, good coverage-guided fuzzing
- **Cons**: May miss some memory bugs
- **Use case**: Quick fuzzing runs, CI/CD pipelines

### 2. AFL++ with AddressSanitizer
- **Pros**: Catches memory bugs (use-after-free, buffer overflows, etc.)
- **Cons**: Slower execution, higher memory usage
- **Use case**: Thorough testing, bug hunting

## Fuzzing Results

Results are saved in the `fuzz-outputs/` directory:
- `fuzzer-asan/queue/` - Interesting test cases found (ASAN mode)
- `fuzzer-asan/crashes/` - Crashes discovered (ASAN mode)
- `fuzzer-asan/hangs/` - Timeout cases (ASAN mode)
- `fuzzer-no-asan/queue/` - Interesting test cases found (no ASAN mode)
- `fuzzer-no-asan/crashes/` - Crashes discovered (no ASAN mode)
- `fuzzer-no-asan/hangs/` - Timeout cases (no ASAN mode)

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
2. **Runs AFL++ without ASAN** (30 min timeout)
3. **Runs AFL++ with ASAN** (30 min timeout)
4. **Checks for crashes** in both runs
5. **Uploads artifacts** with fuzzing results
6. **Comments on PRs** with results summary

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
- **Vec(i32)** - Integer vector operations
- **Vec(char*)** - String pointer vector operations  
- **Vec(Str)** - String vector operations
- **Str** - String operations

Each harness covers 50+ functions with comprehensive test cases.

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
- AFL++ without ASAN: ~1000-5000 execs/sec
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
  bash -c "/usr/local/bin/build_afl.sh && afl-fuzz -t 5000 -m 200 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness"
```

### Parallel Fuzzing
```bash
# Run multiple fuzzer instances
docker run -it --rm \
  -v $(pwd)/fuzz-outputs:/workspace/fuzz/outputs \
  misra-fuzz \
  bash -c "
    /usr/local/bin/build_afl.sh
    cd build-afl
    afl-fuzz -M fuzzer1 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness &
    afl-fuzz -S fuzzer2 -i /usr/local/share/misra-fuzz/inputs -o ../fuzz/outputs ./FuzzHarness &
    wait
  "
```

## Contributing

When adding new functions to the library:
1. Add corresponding test cases to the appropriate harness
2. Update the function enumeration in the harness header
3. Test with both AFL++ approaches
4. Ensure no crashes in CI fuzzing
