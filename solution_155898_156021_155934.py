import subprocess
import json
import config

# NOTE: Setup solution according to SETUP.md

def solve(min_support, min_confidence, verbose=False):
    result = subprocess.run(
        ["./main.exe", str(min_support), str(min_confidence), config.datapath, "1" if verbose else "0"],
        capture_output=True,
        text=True
    )

    print(result.stdout)
    
    rules = []
    return rules


if __name__ == "__main__":
    solve(config.min_support, config.min_confidence, verbose=False)