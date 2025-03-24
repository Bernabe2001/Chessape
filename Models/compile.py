import subprocess
import sys

def compile_versions():
    # Compile DEBUG versions
    for depth in [2, 4, 6, 8]:
        cmd = [
            "g++",
            "-std=c++17",
            "-O3",
            "-D", "DEBUG",
            "-D", f"DEPTH={depth}",
            "-march=native",
            "-flto",
            "-o", f"Chessape_1.0_{depth}_DEBUG",
            "Chessape_1.0.cpp"
        ]
        print(f"Compiling: {' '.join(cmd)}")
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"Failed to compile DEBUG version with DEPTH={depth}", file=sys.stderr)
            return False
    
    # Compile release versions
    for depth in [2, 4, 6, 8]:
        cmd = [
            "g++",
            "-std=c++17",
            "-O3",
            "-D", f"DEPTH={depth}",
            "-march=native",
            "-flto",
            "-o", f"Chessape_1.0_{depth}",
            "Chessape_1.0.cpp"
        ]
        print(f"Compiling: {' '.join(cmd)}")
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"Failed to compile release version with DEPTH={depth}", file=sys.stderr)
            return False

    # Compile TEST versions
    for depth in [2, 4, 6, 8]:
        cmd = [
            "g++",
            "-std=c++17",
            "-O3",
            "-D", "TEST",
            "-D", f"DEPTH={depth}",
            "-march=native",
            "-flto",
            "-o", f"Chessape_1.0_{depth}_TEST",
            "Chessape_1.0.cpp"
        ]
        print(f"Compiling: {' '.join(cmd)}")
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"Failed to compile TEST version with DEPTH={depth}", file=sys.stderr)
            return False
    return True

if __name__ == "__main__":
    if compile_versions():
        print("All versions compiled successfully!")
    else:
        print("Compilation failed for one or more versions.", file=sys.stderr)
        sys.exit(1)