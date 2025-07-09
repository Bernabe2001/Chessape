import torch
from nnue_model import NNUE

def export_weights(model, path="weights.txt"):
    with open(path, "w") as f:
        layers = model.model
        linear_layers = [layer for layer in layers if isinstance(layer, torch.nn.Linear)]

        for layer in linear_layers:
            w = layer.weight.detach().cpu().numpy()
            b = layer.bias.detach().cpu().numpy()

            for row in w:
                f.write(" ".join(map(str, row)) + "\n")
            # ✅ Bias in a single line (not misaligned like a row of weights)
            f.write(" ".join(map(str, b)) + "\n")

    print(f"Weights saved to {path}")

if __name__ == "__main__":
    model = NNUE()
    model.load_state_dict(torch.load("trained_model.pt"))
    export_weights(model)




