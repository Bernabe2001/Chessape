import pexpect
import chess
import subprocess


def aimove(moves):
    stockfish.sendline('setoption name Skill Level value 3')
    stockfish.sendline(f"position startpos moves {' '.join(moves)}")
    stockfish.sendline('go depth 10')
    stockfish.expect(r'bestmove (\S+)', timeout=30)
    stockfish_move = stockfish.match.group(1).decode()
    return stockfish_move

def aimove2(moves, engine_executable="../Min_Max/MinMax", input_file="../Min_Max/input.txt"):
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

def check_for_mate(moves):
    stockfish.sendline(f"position startpos moves {' '.join(moves)}")
    stockfish.sendline('go depth 2')
    while True:
        line = stockfish.readline().strip().decode('utf-8')
        if 'score mate 0' in line:
            return True
        if "bestmove" in line:
            return False

def detect_threefold_repetition(moves):
    """
    Given a list of moves in long algebraic notation, returns True if
    a threefold repetition has occurred at any point in the sequence.
    """
    board = chess.Board()
    position_counts = {}

    # Record the initial position
    initial_epd = board.epd()
    position_counts[initial_epd] = 1

    for move in moves:
        board.push_uci(move)
        current_epd = board.epd()
        position_counts[current_epd] = position_counts.get(current_epd, 0) + 1
        if position_counts[current_epd] >= 3:
            return True

    return False

# Start Stockfish process
stockfish = pexpect.spawn('./Stockfish/Stockfish/src/stockfish')

# Set initial position
for level in range(1, 21):
    ai_wins = 0
    st_wins = 0
    print(f"Level {level}")
    for i in range(5):
        print(f"Game {i}")
        # Initialize Stockfish
        stockfish.sendline('uci')
        stockfish.expect('uciok')

        # Start a new game
        stockfish.sendline('ucinewgame')
        stockfish.sendline('isready')
        stockfish.expect('readyok')
        moves = []
        while True:
            # AI move
            ai_move = aimove2(moves) # input("Your AI's move: ")  # Replace with your AI's logic
            print(f"AI move: {ai_move}")
            moves.append(ai_move)
            if check_for_mate(moves):
                print("Result: AI wins")
                print(f"Game {' '.join(moves)}")
                ai_wins +=1
                break
            elif detect_threefold_repetition(moves):
                print("Result: Draw")
                print(f"Game {' '.join(moves)}")
                ai_wins += 0.5
                st_wins += 0.5
                break
            stockfish.sendline('setoption name Skill Level value ' + str(level))
            stockfish.sendline(f"position startpos moves {' '.join(moves)}")
            stockfish.sendline('go depth 10')
            stockfish.expect(r'bestmove (\S+)', timeout=30)
            stockfish_move = stockfish.match.group(1).decode()
            moves.append(stockfish_move)
            print(f"Stockfish move: {stockfish_move}")
            if check_for_mate(moves):
                print("Result: Stockfish wins")
                print(f"Game {' '.join(moves)}")
                st_wins += 1
                break
            elif detect_threefold_repetition(moves):
                print("Result: Draw")
                print(f"Game {' '.join(moves)}")
                ai_wins += 0.5
                st_wins += 0.5
                break
    print(f"Level {level} finished")
    print('AI wins:', ai_wins)
    print('Stockfish wins:', st_wins)

# PLAY A GAME IN CL
# uci
# isready
# ucinewgame
# position startpos moves e2e4 e7e5
# go depth 10

# print(stockfish.before)Ç

# setoption name Skill Level value 10
# setoption name Depth value 5