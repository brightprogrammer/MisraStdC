import os
import subprocess
import shutil
from pathlib import Path

# List of file extensions for C and C++ source files
C_CPP_EXTENSIONS = {'.c', '.cpp', '.h', '.hpp', '.cc', '.cxx', '.hxx'}

def check_clang_format_version(clang_format_path):
    """Check if clang-format is version 20 or higher."""
    try:
        result = subprocess.run([clang_format_path, '--version'], capture_output=True, text=True, check=True)
        version_output = result.stdout.strip()
        print(f"Found clang-format: {version_output}")
        
        # Extract version number from output like "clang-format version 20.1.8"
        import re
        version_match = re.search(r'version (\d+)\.(\d+)\.(\d+)', version_output)
        if version_match:
            major_version = int(version_match.group(1))
            if major_version < 20:
                print(f"❌ Error: clang-format version {major_version} is not supported.")
                print("This project requires clang-format version 20 or higher.")
                print("\nTo install clang-format 20:")
                print("  Ubuntu/Debian: sudo apt-get install clang-format-20")
                print("  macOS: brew install llvm@20")
                print("  Or download from: https://releases.llvm.org/download.html")
                print("\nAfter installation, ensure clang-format-20 is in your PATH or create a symlink:")
                print("  sudo ln -sf /usr/bin/clang-format-20 /usr/bin/clang-format")
                exit(1)
            else:
                print(f"✅ clang-format version {major_version} is supported.")
        else:
            print("⚠️  Warning: Could not determine clang-format version, proceeding anyway.")
    except subprocess.CalledProcessError as e:
        print(f"Error checking clang-format version: {e}")
        exit(1)

def find_clang_format():
    """Find the path to the clang-format executable."""
    clang_format_path = shutil.which('clang-format')  # Works on both Windows and POSIX systems
    if clang_format_path:
        print(f"Using clang-format at: {clang_format_path}")
        check_clang_format_version(clang_format_path)
        return clang_format_path
    else:
        print("Error: clang-format not found. Please ensure clang-format is installed and in your PATH.")
        print("\nTo install clang-format 20:")
        print("  Ubuntu/Debian: sudo apt-get install clang-format-20")
        print("  macOS: brew install llvm@20")
        print("  Or download from: https://releases.llvm.org/download.html")
        exit(1)

def find_clang_format_dir(start_dir):
    """Find the nearest directory containing a .clang-format file."""
    current_dir = Path(start_dir).resolve()
    
    while current_dir != current_dir.parent:
        clang_format_path = current_dir / ".clang-format"
        if clang_format_path.exists():
            return clang_format_path
        current_dir = current_dir.parent
    
    return None  # No .clang-format found up to the root

def parse_gitignore(root_dir):
    """Parse the .gitignore file and return a list of patterns to ignore."""
    gitignore_path = Path(root_dir) / '.gitignore'
    ignored_paths = set()
    
    if gitignore_path.exists():
        with open(gitignore_path, 'r') as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip()
                if line and not line.startswith('#'):  # Skip comments and empty lines
                    ignored_paths.add(line)
    
    return ignored_paths

def is_ignored(file_path, ignored_paths):
    """Check if the file or directory should be ignored based on .gitignore patterns."""
    relative_path = str(file_path.relative_to(os.getcwd()))
    
    for pattern in ignored_paths:
        if relative_path.startswith(pattern):  # This assumes a simple matching, ignoring more complex patterns
            return True
    return False

def format_file(file_path, clang_format_path):
    """Apply clang-format to the file using the specified .clang-format."""
    try:
        # Ensure the file path is valid and belongs to C/C++ files
        if file_path.suffix.lower() in C_CPP_EXTENSIONS:
            subprocess.run([clang_format_path, '-i', '-style=file', str(file_path)], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error formatting {file_path}: {e}")

def traverse_and_format(root_dir, clang_format_path, ignored_paths):
    """Traverse all files in the directory tree and apply clang-format."""
    for root, dirs, files in os.walk(root_dir):
        # Modify dirs in-place to ignore certain directories (skip traversal in them)
        dirs[:] = [d for d in dirs if not is_ignored(Path(root) / d, ignored_paths)]
        
        for file in files:
            file_path = Path(root) / file
            if is_ignored(file_path, ignored_paths):
                continue  # Skip files that are ignored

            clang_format_dir = find_clang_format_dir(file_path)
            if clang_format_dir:
                format_file(file_path, clang_format_path)

if __name__ == "__main__":
    # Find the clang-format executable
    clang_format_path = find_clang_format()

    # Get the current working directory (the directory from which the script is executed)
    root_directory = os.getcwd()
    
    # Parse the .gitignore file
    ignored_paths = parse_gitignore(root_directory)
    
    # Traverse the project directory and format files
    traverse_and_format(root_directory, clang_format_path, ignored_paths)
