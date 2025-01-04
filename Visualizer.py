import matplotlib.pyplot as plt
from PIL import Image
import numpy as np
import chess
import chess.svg
from io import BytesIO
import cairosvg
import subprocess

# Mapping FEN and the best move to a visualized board
def visualize_chessboard(fen, best_move, evaluation, output_file="visualizer.png"):
    # Parse the board from FEN
    board = chess.Board(fen)

    # Generate the chessboard as an SVG image
    board_svg = chess.svg.board(board, arrows=[
        (chess.parse_square(best_move[:2]), chess.parse_square(best_move[2:]))
    ])

    # Convert the SVG to PNG using cairosvg
    png_data = cairosvg.svg2png(bytestring=board_svg.encode('utf-8'))

    # Load the PNG image into PIL
    img = Image.open(BytesIO(png_data))

    # Convert the image to a NumPy array for plotting
    img_array = np.array(img)

    # Create a figure and display the board
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.imshow(img_array)
    ax.axis('off')

    # Add evaluation and move information at the top
    ax.text(0.5, 1.05, f"Best Move: {best_move}, Evaluation: {evaluation}",
            fontsize=16, transform=ax.transAxes, ha='center', color='white')

    # Save the image to a file
    img.save(output_file)
    print(f"Visualization saved as {output_file}")

# Run the C++ engine and capture its output
def run_chess_engine(engine_executable, input_file):
    # Run the C++ chess engine and capture its output
    result = subprocess.run([engine_executable, input_file], capture_output=True, text=True, check=True)
    return result.stdout.strip().split("\n")

# Integration logic
def main():
    engine_executable = "./MinMax"  # Path to the compiled C++ chess engine
    input_file = "input.txt"

    # Run the engine
    engine_output = run_chess_engine(engine_executable, input_file)

    # Parse the output
    fen = engine_output[1]  # Extract FEN from the second line
    best_move = engine_output[2].split(": ")[1]  # Assuming "Best Move: g1f2"
    evaluation = int(engine_output[3].split(": ")[1])  # Assuming "Evaluation: 109"

    # Visualize the results
    visualize_chessboard(fen, best_move, evaluation)

if __name__ == "__main__":
    main()
