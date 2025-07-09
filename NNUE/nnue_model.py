import torch
import torch.nn as nn

class NNUE(nn.Module):
    def __init__(self):
        super(NNUE, self).__init__()
        self.model = nn.Sequential(
            nn.Linear(768, 512),
            nn.ReLU(),
            nn.Linear(512, 256),
            nn.ReLU(),
            nn.Linear(256, 1)  # Output: centipawn score
        )

    def forward(self, x):
        return self.model(x).squeeze(-1)  # Shape: (batch_size,)