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

    rules = []
    if result.returncode != 0:
        raise RuntimeError(result.stderr or result.stdout or f"main.exe exited with code {result.returncode}")

    for line in result.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        rules.append(json.loads(line))

    if verbose:
        for rule in rules:
            print(f"{rule['A']}=>{rule['B']} Support: {rule['supp']}, Confidence: {rule['conf']}")

    return rules


if __name__ == "__main__":
    solve(config.min_support, config.min_confidence, verbose=False)
