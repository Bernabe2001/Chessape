import subprocess
import sys
import os

os.makedirs("bin", exist_ok=True)

def parse_depth_input(s):
    s = s.strip()
    # Check for range format x-y
    if '-' in s:
        try:
            parts = s.split('-')
            if len(parts) != 2:
                raise ValueError
            start = int(parts[0].strip())
            end = int(parts[1].strip())
        except ValueError:
            print("Invalid range format. Please provide two integers separated by a dash (e.g. 2-10).", file=sys.stderr)
            sys.exit(1)
        if start > end:
            print("Invalid range: start should be less than or equal to end.", file=sys.stderr)
            sys.exit(1)
        return list(range(start, end + 1))
    # Check for comma-separated list
    elif ',' in s:
        try:
            return [int(x.strip()) for x in s.split(',')]
        except ValueError:
            print("Invalid list format. Please provide comma separated integers (e.g. 2,7,10).", file=sys.stderr)
            sys.exit(1)
    else:
        # Assume it's a single integer
        try:
            return [int(s)]
        except ValueError:
            print("Invalid input for depth. Please provide a range (x-y) or comma separated integers.", file=sys.stderr)
            sys.exit(1)

def compile_versions(base_filename, depths, compile_normal, compile_debug, compile_test, max_depth):
    for depth in depths:
        if compile_debug:
            cmd = [
                "g++",
                "-std=c++17",
                "-O3",
                "-D", "DEBUG",
                "-D", f"DEPTH={depth}",
                "-D", f"EXTRA={max_depth[1:]}",
                "-march=native",
                "-flto",
                "-o", f"bin/{base_filename}_{depth}{max_depth}_DEBUG",
                f"{base_filename}.cpp"
            ]
            print("Compiling:", ' '.join(cmd))
            result = subprocess.run(cmd)
            if result.returncode != 0:
                print(f"Failed to compile DEBUG version for {base_filename} with DEPTH={depth}", file=sys.stderr)
                return False

        if compile_normal:
            cmd = [
                "g++",
                "-std=c++17",
                "-O3",
                "-D", f"DEPTH={depth}",
                "-D", f"EXTRA={max_depth[1:]}",
                "-march=native",
                "-flto",
                "-o", f"bin/{base_filename}_{depth}{max_depth}",
                f"{base_filename}.cpp"
            ]
            print("Compiling:", ' '.join(cmd))
            result = subprocess.run(cmd)
            if result.returncode != 0:
                print(f"Failed to compile NORMAL version for {base_filename} with DEPTH={depth}", file=sys.stderr)
                return False

        if compile_test:
            cmd = [
                "g++",
                "-std=c++17",
                "-O3",
                "-D", "TEST",
                "-D", f"DEPTH={depth}",
                "-D", f"EXTRA={max_depth[1:]}",
                "-march=native",
                "-flto",
                "-o", f"bin/{base_filename}_{depth}{max_depth}_TEST",
                f"{base_filename}.cpp"
            ]
            print("Compiling:", ' '.join(cmd))
            result = subprocess.run(cmd)
            if result.returncode != 0:
                print(f"Failed to compile TEST version for {base_filename} with DEPTH={depth}", file=sys.stderr)
                return False
    return True

if __name__ == "__main__":
    depth_input = input("Enter depths to compile (either a closed interval x-y or a comma separated list, e.g. 2,7,10): ")
    depths = parse_depth_input(depth_input)

    file_input = input("Enter the base file name(s) to compile (separated by a space, e.g. Chessape_1.0): ")
    base_files = file_input.split()

    compile_choice = input("Which versions do you want to compile? Options: NORMAL, DEBUG, TEST, or ALL: ").strip().upper()
    compile_normal = compile_debug = compile_test = False
    if compile_choice == "ALL":
        compile_normal = compile_debug = compile_test = True
    elif compile_choice == "NORMAL":
        compile_normal = True
    elif compile_choice == "DEBUG":
        compile_debug = True
    elif compile_choice == "TEST":
        compile_test = True
    else:
        print("Invalid compile version option.", file=sys.stderr)
        sys.exit(1)

    max_depth = ""
    if file_input == "Chessape_1.2":
        max_depth = "_" + input("Enter maximum extra depth: ")

    for base_file in base_files:
        if compile_versions(base_file, depths, compile_normal, compile_debug, compile_test, max_depth):
            print(f"All selected variants compiled successfully for {base_file}!")
        else:
            print(f"Compilation failed for {base_file}.", file=sys.stderr)
            sys.exit(1)
