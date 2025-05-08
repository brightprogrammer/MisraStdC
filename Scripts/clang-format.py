import os
import subprocess
import shutil
from pathlib import Path

# List of file extensions for C and C++ source files
C_CPP_EXTENSIONS = {'.c', '.cpp', '.h', '.hpp', '.cc', '.cxx', '.hxx'}

def find_clang_format():
    """Find the path to the clang-format executable."""
    clang_format_path = shutil.which('clang-format')  # Works on both Windows and POSIX systems
    if clang_format_path:
        print(f"Using clang-format at: {clang_format_path}")
        return clang_format_path
    else:
        print("Error: clang-format not found. Please ensure clang-format is installed and in your PATH.")
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
