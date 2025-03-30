#!/usr/bin/env python3

import re

def extract_fens(pgn_text):
    # Use regex to extract the FEN string from each [FEN "..."] header.
    fen_pattern = r'\[FEN\s+"([^"]+)"\]'
    fens = re.findall(fen_pattern, pgn_text)
    return fens

def main():
    file_name = input("Enter file to parser (pgn to fen): ").strip()
    try:
        with open(file_name, "r", encoding="utf-8") as file:
            pgn_text = file.read()
    except FileNotFoundError:
        print(f"{file_name} not found.")
        return

    fens = extract_fens(pgn_text)

    # Generate the output text
    output_lines = []
    for index, fen in enumerate(fens, start=1):
        output_lines.append(f"Test {index}:\n{fen}\n")
    output_text = "\n".join(output_lines)

    # Overwrite the file with the generated output
    with open(file_name, "w", encoding="utf-8") as file:
        file.write(output_text)
    print(f"File {file_name} has been overwritten with the generated output.")

if __name__ == '__main__':
    main()

