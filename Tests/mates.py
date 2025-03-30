#!/usr/bin/env python3

import subprocess, time, sys, re

def extract_fens(pgn_text):
    """
    Extracts FEN strings from a PGN text using regex.
    """
    fen_pattern = r'\[FEN\s+"([^"]+)"\]'
    fens = re.findall(fen_pattern, pgn_text)
    return fens

def parse_tests(file_path, num_tests):
    """
    Parses the mates file.
    Each test is assumed to consist of two or more lines (header, FEN, etc)
    separated by a blank line.
    Only the first num_tests entries are parsed.
    """
    tests = []
    try:
        with open(file_path, 'r', encoding="utf-8") as f:
            content = f.read().strip()
        # Split the file into entries separated by blank lines.
        entries = content.split("\n\n")
        for entry in entries[:num_tests]:
            lines = entry.strip().splitlines()
            if len(lines) >= 2:
                # Assuming that the second line is the FEN string.
                fen = lines[1].strip()
                tests.append(fen)
        return tests
    except Exception as e:
        print("Error reading tests file:", e)
        return []

def run_test(test_fen, engine_path, limit):
    """
    Runs a single test:
    - Starts the engine process.
    - Initializes UCI (waits for "uciok" and "readyok").
    - Sends the test FEN via the 'position' command and then "go".
    - Monitors engine output for the mate score ("2147483647").
    Returns True if the mate score is found, otherwise False.
    """
    print("Testing FEN:", test_fen)
    try:
        process = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,  # combine stderr with stdout
            text=True,
            bufsize=1
        )

        # Initialize UCI mode.
        process.stdin.write("uci\n")
        process.stdin.flush()
        while True:
            line = process.stdout.readline()
            if "uciok" in line:
                break

        process.stdin.write("isready\n")
        process.stdin.flush()
        while True:
            line = process.stdout.readline()
            if "readyok" in line:
                break

        # Send position command with the FEN.
        process.stdin.write(f"position fen {test_fen}\n")
        process.stdin.flush()

        process.stdin.write(f"go wtime 0 btime 0 winc {limit} binc {limit}\n")
        process.stdin.flush()

        found = False
        # Monitor the engine output.
        while True:
            line = process.stdout.readline()
            #print(line)
            if not line:
                break  # End-of-file reached.
            if "Score" in line:
                print(line)
            if "32767" in line:
                found = True
                break
            if "2147483647" in line:
                found = True
                break
            if "bestmove" in line:
                break

        # Cleanly shut down the engine.
        process.stdin.write("quit\n")
        process.stdin.flush()
        process.wait(timeout=1)
        return found

    except Exception as e:
        print("Error running test:", e)
        return False

def parse_levels(levels_input):
    """
    Parses the mate level input string and returns a list of levels.
    Accepts formats like "4", "2,3,4", or "4-8".
    """
    levels_input = levels_input.strip()
    levels = []
    if '-' in levels_input:
        # Handle range format like "4-8"
        parts = levels_input.split('-')
        try:
            start = int(parts[0])
            end = int(parts[1])
            levels = list(range(start, end + 1))
        except ValueError:
            print("Invalid range format.")
    elif ',' in levels_input:
        # Handle comma-separated values like "2,3,4"
        try:
            levels = [int(x.strip()) for x in levels_input.split(',')]
        except ValueError:
            print("Invalid comma-separated format.")
    else:
        try:
            levels = [int(levels_input)]
        except ValueError:
            print("Invalid level input.")
    return levels

def main():
    # Ask user for mate levels.
    mate_levels_input = input("Enter mate level [2-10] (e.g. 4 or 2,3,4 or 4-8): ")
    mate_levels = parse_levels(mate_levels_input)
    if not mate_levels:
        print("No valid mate levels provided. Exiting.")
        sys.exit(1)

    version = input("Enter version: ").strip()
    num_tests_input_str = input("How many tests per level? ").strip()
    limit = input("Enter maximum estimated time to stop: ").strip()
    try:
        num_tests_input = int(num_tests_input_str) if num_tests_input_str else None
    except ValueError:
        print("Invalid number, using all tests.")
        num_tests_input = None

    engine_path = f"../Models/bin/Chessape_{version}_DEBUG"
    results = []  # list to store summary for each level

    for level in mate_levels:
        test_file_path = f"./Data/Mates/Mates_in_{level}.txt"
        # Determine maximum number of tests available in the file.
        try:
            with open(test_file_path, "r", encoding="utf-8") as f:
                file_content = f.read().strip()
            max_tests = len(file_content.split("\n\n")) if file_content else 0
        except FileNotFoundError:
            print(f"File {test_file_path} not found. Skipping level {level}.")
            continue

        if max_tests == 0:
            print(f"No tests found in {test_file_path}. Skipping level {level}.")
            continue

        num_tests = min(max_tests, num_tests_input) if num_tests_input else max_tests

        tests = parse_tests(test_file_path, num_tests)
        total_tests = len(tests)
        if total_tests == 0:
            print(f"No tests parsed for level {level}.")
            continue

        correct = 0
        total_time = 0

        print(f"\nRunning {total_tests} tests for level {level}...")
        for i, test in enumerate(tests, 1):
            start_time = time.time()
            result = run_test(test, engine_path, limit)
            if result:
                correct += 1
            elapsed = time.time() - start_time
            total_time += elapsed
            print(f"Test {i}/{total_tests}: {'Success' if result else 'Fail'} - Time: {elapsed:.2f}s - Accuracy: {correct/i:.1%}")

        accuracy = correct / total_tests if total_tests else 0
        average_time = total_time / total_tests if total_tests else 0

        results.append({
            "Level": level,
            "Cases": total_tests,
            "Accuracy": accuracy,
            "Avg Time": average_time
        })

        print(f"\nLevel {level} results: {correct}/{total_tests} ({accuracy:.1%}) - Avg time: {average_time:.2f} seconds\n")

    # Print summary table.
    if results:
        print("\nSummary Table:")
        print(f"{'Level':<10}{'Cases':<10}{'Accuracy':<15}{'Avg Time (s)':<15}")
        for res in results:
            print(f"{res['Level']:<10}{res['Cases']:<10}{res['Accuracy']*100:<15.1f}{res['Avg Time']:<15.2f}")
    else:
        print("No results to display.")

if __name__ == "__main__":
    main()