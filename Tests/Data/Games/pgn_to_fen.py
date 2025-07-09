import chess
import chess.pgn
import chess.engine
import csv
import os

PGN_FILES = ["Fisher.txt", "Kasparov.txt"]
STOCKFISH_PATH = "../../../../stockfish/stockfish-ubuntu-x86-64-avx2"
OUTPUT_FILE = "train_data.csv"
EVAL_DEPTH = 12
SKIP_MOVES = 1  # Analyze every move

def extract_positions_and_evaluate():
    total_positions = 0

    with open(OUTPUT_FILE, 'w', newline='') as out_file:
        writer = csv.DictWriter(out_file, fieldnames=["fen", "evaluation"])
        writer.writeheader()

        engine = chess.engine.SimpleEngine.popen_uci(STOCKFISH_PATH)

        for pgn_file in PGN_FILES:
            if not os.path.exists(pgn_file):
                print(f"File not found: {pgn_file}")
                continue

            with open(pgn_file) as pgn:
                while True:
                    game = chess.pgn.read_game(pgn)
                    if game is None:
                        break

                    board = game.board()
                    for i, move in enumerate(game.mainline_moves()):
                        board.push(move)
                        if i % SKIP_MOVES != 0:
                            continue

                        fen = board.fen()
                        try:
                            info = engine.analyse(board, chess.engine.Limit(depth=EVAL_DEPTH))
                            score = info["score"].white().score(mate_score=10000)
                            if score is not None:
                                writer.writerow({"fen": fen, "evaluation": score})
                                total_positions += 1
                                if total_positions % 100 == 0:
                                    print(f"{total_positions} positions analyzed...")
                        except Exception as e:
                            print(f"Error evaluating position {fen}: {e}")

        engine.quit()
        print(f"\nDone! Total positions saved: {total_positions}")

if __name__ == "__main__":
    extract_positions_and_evaluate()

        
