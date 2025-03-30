#!/usr/bin/env python3

import pexpect
import chess
import subprocess
import os

# Estimated Stockfish ELO ratings for Skill Levels (approximate)
STOCKFISH_ELO = {
    1: 1350, 2: 1450, 3: 1550, 4: 1650, 5: 1750, 6: 1850, 7: 1950, 8: 2050, 9: 2150,
    10: 2250, 11: 2350, 12: 2450, 13: 2550, 14: 2650, 15: 2750, 16: 2850, 17: 2950,
    18: 3050, 19: 3150, 20: 3250
}

# Simulated ELO range (550 to 1350) using movetime limitation
SIMULATED_ELO = {
    550: {"movetime": 2},
    650: {"movetime": 5},
    750: {"movetime": 10},
    850: {"movetime": 20},
    950: {"movetime": 40},
    1050: {"movetime": 80},
    1150: {"movetime": 150},
    1250: {"movetime": 300},
    1350: {"movetime": 600}
}

def stockfish_move(moves, skill_level, movetime=1000):
    stockfish.sendline(f"setoption name Skill Level value {skill_level}")
    stockfish.sendline(f"position startpos moves {' '.join(moves)}")
    stockfish.sendline(f"go movetime {movetime}")
    stockfish.expect(r'bestmove (\S+)', timeout=60)
    return stockfish.match.group(1).decode()

def model_move(moves):
    """
    Spawns the model engine, sends UCI commands, and gets the best move.
    For simplicity, spawns the model once and reuses it.
    """
    model.sendline(f"position startpos moves {' '.join(moves)}")
    model.sendline("go")
    try:
        model.expect(r'bestmove (\S+)', timeout=60)
    except Exception as e:
        print("Model failed to return move:", e)
        return None
    return model.match.group(1).decode()

def get_move_generic(engine, moves):
    """
    A helper function to get a move from any engine process.
    """
    engine.sendline(f"position startpos moves {' '.join(moves)}")
    engine.sendline("go")
    try:
        index = engine.expect([r'bestmove (\S+)', pexpect.EOF, pexpect.TIMEOUT], timeout=60)
    except Exception as e:
        print("Error waiting for bestmove:", e)
        raise
    if index != 0:
        print("Engine output error or unexpected termination.")
        print("Buffer:", engine.before.decode())
        raise Exception("Engine failed to produce bestmove")
    return engine.match.group(1).decode()

def log_game(moves, mode, result):
    """
    Appends the game moves and result to a log file.
    """
    with open("game_records.txt", "a") as f:
        f.write(f"\n### Game Mode: {mode} ###\n")
        for i, move in enumerate(moves):
            if i % 2 == 0:
                f.write(f"{i//2+1}. {move} ")
            else:
                f.write(f"{move}\n")
        f.write(f"\nResult: {result}\n")

def game_end(moves):
    """
    Evaluates the current game state by applying the list of UCI moves to a new board.
    Returns:
        "WIN" if White (the engine playing white) wins,
        "LOSE" if White loses,
        "DRAW" if the game is drawn,
        "NONE" if the game is not over.
    """
    board = chess.Board()
    for move in moves:
        try:
            board.push_uci(move)
        except Exception as e:
            print(f"Invalid move encountered: {move}")
            raise
    if board.is_game_over():
        result = board.result()  # "1-0", "0-1", or "1/2-1/2"
        if result == "1-0":
            return "WIN"
        elif result == "0-1":
            return "LOSE"
        else:
            return "DRAW"
    if board.can_claim_fifty_moves():
        return "DRAW"
    if board.can_claim_threefold_repetition():
            return "DRAW"
    return "NONE"

def evaluate_mode(engine_path):
    """
    Plays a series of games between the custom engine (from engine_path)
    and Stockfish. The engine alternates playing White and Black.
    The score is updated based on game outcomes (note: game_end always returns
    result from White's perspective).
    """
    min_elo, max_elo = 550, 3250
    current_elo = min_elo
    ai_wins, st_wins = 0, 0

    # Start with engine playing White; will alternate each game.
    engine_is_white = True

    while True:
        print(f"\nTesting engine vs. Stockfish at {current_elo} ELO")
        print("Engine is playing", "White" if engine_is_white else "Black")
        moves = []
        # Start new game for both engines
        stockfish.sendline("ucinewgame")
        stockfish.sendline("isready")
        stockfish.expect("readyok")

        model.sendline("ucinewgame")
        model.sendline("isready")
        model.expect("readyok")

        # Game loop: alternate moves depending on engine color.
        if engine_is_white:
            while True:
                # Engine (White) move
                engine_move = model_move(moves)
                if not engine_move:
                    print("Engine failed to generate a move.")
                    return
                moves.append(engine_move)
                print(engine_move)
                result = game_end(moves)
                if result != "NONE":
                    # result from white's perspective: WIN means engine win.
                    if result == "WIN":
                        ai_wins += 1
                    elif result == "LOSE":
                        st_wins += 1
                    else:
                        ai_wins += 0.5
                        st_wins += 0.5
                    break

                # Stockfish (Black) move
                if current_elo >= 1350:
                    skill_level = min(STOCKFISH_ELO, key=lambda x: abs(STOCKFISH_ELO[x] - current_elo))
                    sf_move = stockfish_move(moves, skill_level=skill_level, movetime=1000)
                else:
                    params = SIMULATED_ELO.get(current_elo)
                    sf_move = stockfish_move(moves, skill_level=1, movetime=params["movetime"])
                moves.append(sf_move)
                print(sf_move)
                result = game_end(moves)
                if result != "NONE":
                    if result == "WIN":
                        ai_wins += 1
                    elif result == "LOSE":
                        st_wins += 1
                    else:
                        ai_wins += 0.5
                        st_wins += 0.5
                    break
        else:
            # Engine is Black. Stockfish plays White.
            while True:
                # Stockfish (White) move
                if current_elo >= 1350:
                    skill_level = min(STOCKFISH_ELO, key=lambda x: abs(STOCKFISH_ELO[x] - current_elo))
                    sf_move = stockfish_move(moves, skill_level=skill_level, movetime=1000)
                else:
                    params = SIMULATED_ELO.get(current_elo)
                    sf_move = stockfish_move(moves, skill_level=1, movetime=params["movetime"])
                moves.append(sf_move)
                print(sf_move)
                result = game_end(moves)
                if result != "NONE":
                    # result from white's perspective: WIN means Stockfish wins.
                    if result == "WIN":
                        st_wins += 1
                    elif result == "LOSE":
                        ai_wins += 1
                    else:
                        ai_wins += 0.5
                        st_wins += 0.5
                    break

                # Engine (Black) move
                engine_move = model_move(moves)
                if not engine_move:
                    print("Engine failed to generate a move.")
                    return
                moves.append(engine_move)
                print(engine_move)
                result = game_end(moves)
                if result != "NONE":
                    if result == "WIN":
                        st_wins += 1
                    elif result == "LOSE":
                        ai_wins += 1
                    else:
                        ai_wins += 0.5
                        st_wins += 0.5
                    break

        log_game(moves, "Evaluation", result)
        #print(moves)
        print(f"Score - Engine: {ai_wins}, Stockfish: {st_wins}")

        # Toggle the engine's color for the next game.
        engine_is_white = not engine_is_white

        # Adjust the ELO boundaries based on score difference.
        if ai_wins >= st_wins + 2:
            min_elo = current_elo
            current_elo += 100
            ai_wins, st_wins = 0, 0
        elif st_wins >= ai_wins + 2:
            max_elo = current_elo
            if current_elo > 550:
                estimated_elo = (min_elo + max_elo) // 2
                print(f"\nEstimated Engine ELO: {estimated_elo}")
                return
            else:
                print("Estimated ELO is below 550")
                return

def match_mode(engine_path1, engine_path2):
    """
    Plays a series of matches between two engines.
    The function asks how many matches to run, then for each match the engines alternate colors.
    Game outcomes are interpreted using game_end (which is from White’s perspective).
    After all matches, the win percentages for each engine (and draws) are printed.
    """
    num_matches = int(input("Enter number of matches to play: "))
    # Spawn the two engines.
    engine1 = pexpect.spawn(engine_path1)
    engine2 = pexpect.spawn(engine_path2)

    engine1_wins = 0
    engine2_wins = 0
    draws = 0

    for i in range(num_matches):
        print(f"\nMatch {i+1}:")
        # Alternate colors: even-numbered match -> engine1 is White; odd-numbered -> engine2 is White.
        if i % 2 == 0:
            print("Engine 1 is White, Engine 2 is Black")
            white_engine = engine1
            black_engine = engine2
        else:
            print("Engine 2 is White, Engine 1 is Black")
            white_engine = engine2
            black_engine = engine1

        # Reset both engines.
        for eng in (white_engine, black_engine):
            eng.sendline("ucinewgame")
            eng.sendline("isready")
            eng.expect("readyok")

        moves = []
        while True:
            # White moves first.
            move = get_move_generic(white_engine, moves)
            moves.append(move)
            print(move)
            result = game_end(moves)
            if result != "NONE":
                break
            # Black moves.
            move = get_move_generic(black_engine, moves)
            moves.append(move)
            print(move)
            result = game_end(moves)
            if result != "NONE":
                break
        # Interpret result from White's perspective.
        if result == "WIN":
            # White wins.
            if white_engine == engine1:
                engine1_wins += 1
                print("Engine 1 wins")
            else:
                engine2_wins += 1
                print("Engine 2 wins")
        elif result == "LOSE":
            # White loses (so Black wins).
            if white_engine == engine1:
                engine2_wins += 1
                print("Engine 2 wins")
            else:
                engine1_wins += 1
                print("Engine 1 wins")
        else:
            draws += 1
            print("Game drawn")

    print("\nMatch result: " + f"{engine1_wins + 0.5*draws}" + " - " + f"{engine1_wins + 0.5*draws}")
    print(f"Engine 1 wins: {engine1_wins} ({(engine1_wins / num_matches) * 100:.2f}%)")
    print(f"Engine 2 wins: {engine2_wins} ({(engine2_wins / num_matches) * 100:.2f}%)")
    print(f"Draws: {draws} ({(draws / num_matches) * 100:.2f}%)")

def main():
    global stockfish
    stockfish_executable = os.path.abspath("../../stockfish/stockfish-ubuntu-x86-64-avx2")
    stockfish = pexpect.spawn(stockfish_executable)

    var = True
    while (var):
        mode = input("Choose mode 'evaluation' (or 'e') for engine vs. Stockfish, 'match' (or 'm') for model vs. model match): ").strip().lower()
        if mode == "e" or mode == "eval" or mode == "evaluation":
            engine_name = input("Enter the name of the engine (e.g. Chessape_1.2_4_5): ").strip()
            engine_path = os.path.abspath(os.path.join("..", "Models/bin", engine_name))
            global model
            model = pexpect.spawn(engine_path)
            evaluate_mode(engine_path)
            var = False
        elif (mode == "match" or mode == "m"):
            engine_name1 = input("Enter the name of the first engine (e.g. Chessape_1.0_4): ").strip()
            engine_path1 = os.path.abspath(os.path.join("..", "Models/bin", engine_name1))
            engine_name2 = input("Enter the name of the second engine (e.g. Chessape_1.2_2_5): ").strip()
            engine_path2 = os.path.abspath(os.path.join("..", "Models/bin", engine_name2))
            match_mode(engine_path1, engine_path2)
            var = False
        else:
            print("Invalid mode selected.")
    return

if __name__ == "__main__":
    main()


