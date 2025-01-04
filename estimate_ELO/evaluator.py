import pexpect

ai_wins = 0
st_wins = 0

def aimove(moves):
    stockfish.sendline('setoption name Skill Level value 20')
    stockfish.sendline(f"position startpos moves {' '.join(moves)}")
    stockfish.sendline('go depth 10')
    stockfish.expect(r'bestmove (\S+)', timeout=30)
    stockfish_move = stockfish.match.group(1).decode()
    return stockfish_move

def check_for_mate(moves):
    stockfish.sendline(f"position startpos moves {' '.join(moves)}")
    stockfish.sendline('go depth 2')
    while True:
        line = stockfish.readline().strip().decode('utf-8')
        if 'score mate 0' in line:
            return True
        if "bestmove" in line:
            return False

# Start Stockfish process
stockfish = pexpect.spawn('./Stockfish/Stockfish/src/stockfish')

# Set initial position
for _ in range(10):
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
        ai_move = aimove(moves) # input("Your AI's move: ")  # Replace with your AI's logic
        print(f"AI move: {ai_move}")
        moves.append(ai_move)
        if check_for_mate(moves):
            print("Result: AI wins")
            print(f"Game {' '.join(moves)}")
            ai_wins +=1
            break
        stockfish.sendline('setoption name Skill Level value 20')
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