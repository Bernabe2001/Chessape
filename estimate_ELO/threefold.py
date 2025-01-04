import chess

def detect_threefold_repetition(moves):
    """
    Given a list of moves in long algebraic notation, returns True if
    a threefold repetition has occurred at any point in the sequence.
    """
    board = chess.Board()

    # Dictionary to count occurrences of positions (based on EPD for ignoring move counters)
    position_counts = {}

    # Record the initial position
    initial_epd = board.epd()
    position_counts[initial_epd] = 1

    for move in moves:
        # Push the move in UCI format (e.g. "e2e4", "g8f6", etc.)
        board.push_uci(move)

        # Get the “EPD” string (piece placement, side to move, castling, en-passant)
        current_epd = board.epd()

        # Count how many times we've seen this position
        position_counts[current_epd] = position_counts.get(current_epd, 0) + 1

        # Check if any position is repeated 3 or more times
        if position_counts[current_epd] >= 3:
            return True

    # If we finish all moves without a 3rd repetition, return False
    return False


if __name__ == "__main__":
    # Example sequence that leads to a position repeated 3 times
    # In reality, you'd have a longer sequence or a real game
    moves_example = [
        "b1c3", "b8c6", "c3b1", "c6b8", "b1c3", "b8c6", "c3b1", "c6b8", "b1c3", "b8c6", "c3b1", "c6b8", "b1c3", "b8c6", "c3b1", "c6b8"
        # (some moves that force the same position to re-occur)
        # ...
    ]

    if detect_threefold_repetition(moves_example):
        print("Threefold repetition has occurred!")
    else:
        print("No threefold repetition in the given sequence.")
