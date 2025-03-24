import subprocess, time, select, sys

# Ask user for mate level and number of tests.
mate_level = input("Enter mate level (2, 3, or 4): ").strip()

# Map mate level to engine executable and test file.
if mate_level == "2":
    engine_path = "../Models/Chessape_1.0_4_DEBUG"
    test_file_path = "./Data/Mates/Mate_in_2.txt"
    max_tests = 221
elif mate_level == "3":
    engine_path = "../Models/Chessape_1.0_6_DEBUG"
    test_file_path = "./Data/Mates/Mate_in_3.txt"
    max_tests = 459
elif mate_level == "4":
    engine_path = "../Models/Chessape_1.0_8_DEBUG"
    test_file_path = "./Data/Mates/Mate_in_4.txt"
    max_tests = 445
else:
    print("Invalid mate level. Using mate in 3.")
    engine_path = "../Models/Chessape_1.0_6_DEBUG"
    test_file_path = "./Data/Mates/Mate_in_2.txt"
    max_tests = 459

num_tests_input = input(f"How many tests? ({max_tests} tests available) ").strip()
try:
    num_tests = min(max_tests, int(num_tests_input)) if num_tests_input else max_tests
except ValueError:
    print("Invalid number. Using all tests from file.")
    num_tests = max_tests

def parse_tests(file_path):
    """
    Parses the mates file.
    Each test is assumed to consist of three lines (header, FEN, move list)
    separated by a blank line. The FEN is on the second line.
    """
    tests = []
    try:
        with open(file_path, 'r') as f:
            content = f.read().strip()
        # Split the file into entries separated by blank lines.
        entries = content.split("\n\n")
        for entry in entries[:num_tests]:
            lines = entry.strip().splitlines()
            if len(lines) >= 2:
                fen = lines[1].strip()
                tests.append(fen)
        return tests
    except Exception as e:
        print("Error reading tests file:", e)
        return []

def run_test(test_fen):
    """
    Runs a single test by starting the engine, initializing UCI,
    sending the test FEN, and scanning output for the mate score.
    Returns True if "2147483647" is found in the engine output.
    """
    print("Testing FEN:", test_fen)
    try:
        # Start the engine process.
        process = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,  # combine stderr with stdout
            text=True,
            bufsize=1
        )

        # Send UCI initialization command.
        process.stdin.write("uci\n")
        process.stdin.flush()
        # Wait for uciok to confirm UCI mode is set.
        while True:
            line = process.stdout.readline()
            if "uciok" in line:
                break

        # Send isready and wait for readyok.
        process.stdin.write("isready\n")
        process.stdin.flush()
        while True:
            line = process.stdout.readline()
            if "readyok" in line:
                break

        # Send the position command with the test FEN.
        process.stdin.write(f"position fen {test_fen}\n")
        process.stdin.flush()

        process.stdin.write("go\n")
        process.stdin.flush()

        # Now, check engine output for the mate result.
        found = False
        # Wait for the engine to output the mate score without a timeout.
        while True:
            line = process.stdout.readline()
            if not line:
                break  # End-of-file reached.
            if "2147483647" in line:
                found = True
                break

        # Cleanly shut down the engine.
        process.stdin.write("quit\n")
        process.stdin.flush()
        process.wait(timeout=1)
        return found

    except Exception as e:
        print("Error running test:", e)
        return False

def main():
    tests = parse_tests(test_file_path)
    total_tests = len(tests)
    if not total_tests:
        print("No tests found.")
        return

    correct = 0
    total_time = 0

    print(f"Running {total_tests} tests...")
    for i, test in enumerate(tests, 1):
        start_time = time.time()
        result = run_test(test)
        if result:
            correct += 1
        elapsed = time.time() - start_time
        total_time += elapsed
        print(f"Test {i}/{total_tests}: {'Success' if result else 'Fail'} - Time: {elapsed:.2f}s - Accuracy: {correct/i:.1%}")

    average_time = total_time / total_tests if total_tests else 0
    print("\nFinal results:")
    print(f"Correct: {correct} / {total_tests} ({correct/total_tests:.1%})")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Average time per test: {average_time:.2f} seconds")

if __name__ == "__main__":
    main()





