import subprocess
import chess

def get_best_move_from_moves(moves, engine_executable="./MinMax", input_file="input.txt", output_file="visualizer.png"):
    """
    Given a list of chess moves in UCI format, generates the FEN, runs the chess engine, visualizes the board,
    and returns the best move.

    :param moves: List of strings representing chess moves in UCI format (e.g., ["e2e4", "e7e5"]).
    :param engine_executable: Path to the compiled C++ chess engine executable.
    :param input_file: Path to the input file for the chess engine.
    :param output_file: Path to save the visualized chessboard image.
    :return: Best move as a string (e.g., "e2e4").
    """
    # Generate FEN from the list of moves
    board = chess.Board()
    for move in moves:
        board.push(chess.Move.from_uci(move))
    fen = board.fen()

    # Write FEN to the input file for the chess engine
    with open(input_file, "w") as f:
        f.write(fen)

    # Run the chess engine
    result = subprocess.run([engine_executable, input_file], capture_output=True, text=True, check=True)
    engine_output = result.stdout.strip().split("\n")

    # Parse the engine output
    best_move = engine_output[2].split(": ")[1]  # Assuming "Best Move: g1f2"
    evaluation = int(engine_output[3].split(": ")[1])  # Assuming "Evaluation: 109"

    return best_move

# Example usage
if __name__ == "__main__":
    moves = ["e2e4", "e7e5", "g1f3"]  # Replace with your list of moves
    best_move = get_best_move_from_moves(moves)
    print(f"Best Move: {best_move}")
    