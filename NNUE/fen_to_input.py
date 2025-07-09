import chess
import numpy as np

# Order: white P, N, B, R, Q, K, black p, n, b, r, q, k
PIECE_TO_INDEX = {
    'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5,
    'p': 6, 'n': 7, 'b': 8, 'r': 9, 'q': 10, 'k': 11
}

def fen_to_input(fen: str) -> np.ndarray:
    board = chess.Board(fen)
    input_vector = np.zeros(768, dtype=np.float32)  # 12 x 64

    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            index = PIECE_TO_INDEX[piece.symbol()]
            input_vector[index * 64 + square] = 1.0

    return input_vector

if __name__ == "__main__":
    test_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    vec = fen_to_input(test_fen)
    print(f"Input shape: {vec.shape}")
    print(f"Non-zero entries: {np.count_nonzero(vec)}")  # should be 32