#!/usr/bin/env python3

def count_islands_and_isolated(n):
    """
    Given an integer n (0 <= n < 256), computes the score for its 8-bit binary representation.
    The score is defined as:
      score = (# of islands) + (# of islands that are isolated)
    where an island is a contiguous group of '1's and an isolated pawn is an island of length 1.

    For example, for the binary '10011000':
      - There are 2 islands: "1" and "11"
      - Only "1" is isolated (length 1)
      - Score = 2 + 1 = 3
    """
    binary = format(n, '08b')
    islands = 0
    isolated = 0
    in_island = False
    current_length = 0

    for char in binary:
        if char == '1':
            if not in_island:
                # Start of a new island
                islands += 1
                in_island = True
                current_length = 1
            else:
                # Continuation of the current island
                current_length += 1
        else:
            if in_island:
                # End of an island: check if it was isolated
                if current_length == 1:
                    isolated += 1
                in_island = False
                current_length = 0

    # If the binary string ended while still in an island, check its length.
    if in_island and current_length == 1:
        isolated += 1

    return islands + isolated

def generate_island_scores():
    """
    Generates a list of 256 numbers, where each index i contains the score
    (islands + isolated islands) for the 8-bit binary representation of i.
    """
    return [-15*max(0,count_islands_and_isolated(i)-1) for i in range(256)]

# Example usage:
if __name__ == "__main__":
    scores = generate_island_scores()
    # For demonstration, print the score for 152, which is '10011000'
    print(scores)

