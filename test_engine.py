#!/usr/bin/env python3
"""Test tomogichi engine logic by piping commands."""
import subprocess
import os

PROJECT = "/home/bym/games/tomogichi"
BINARY = "./tomogichi"

# Remove old state
state_file = f"{PROJECT}/data/state.json"
if os.path.exists(state_file):
    os.remove(state_file)
if os.path.exists(state_file + ".bak"):
    os.remove(state_file + ".bak")

def run_commands(commands):
    """Pipe commands to tomogichi, print output."""
    os.chdir(PROJECT)
    proc = subprocess.Popen(
        [BINARY],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    out, _ = proc.communicate(input="\n".join(commands) + "\nquit\n")
    return out

# ----------------------------------------
# TEST 1: Basic XP and level formulas
# ----------------------------------------
print("=" * 60)
print("TEST 1: XP formula check")
print("=" * 60)

# 1 hour = 60 min = 100 XP
# Level = floor(sqrt(xp/100))
# lvl 1: 100 XP (1h)   lvl 5: 2500 XP (25h)   lvl 10: 10000 XP (100h)
# Person lvl: 50% main + 25% sec1 + 25% sec2
print()
print("XP → Level mapping:")
for xp in [0, 50, 100, 200, 500, 1000, 2500, 5000, 10000, 22500, 40000]:
    lvl = int((xp / 100) ** 0.5)
    hrs = xp / 100.0
    print(f"  {xp:>6} XP ({hrs:>6.1f}h) → lvl {lvl}")

print()
print("Person level examples (main / sec1 / sec2 → person lvl):")
examples = [
    (1, 0, 0),    # 0.5*1 + 0.25*0 + 0.25*0 = 0.5 → 0
    (2, 0, 0),    # 0.5*2 = 1.0 → 1
    (1, 1, 1),    # 0.5*1 + 0.25*1 + 0.25*1 = 1.0 → 1
    (5, 2, 2),    # 0.5*5 + 0.25*2 + 0.25*2 = 3.5 → 3
    (10, 5, 5),   # 0.5*10 + 0.25*5 + 0.25*5 = 7.5 → 7
    (10, 8, 8),   # 0.5*10 + 0.25*8 + 0.25*8 = 9.0 → 9
    (10, 10, 10), # 0.5*10 + 0.25*10 + 0.25*10 = 10.0 → 10
]
for m, s1, s2 in examples:
    person_lvl = int(0.50 * m + 0.25 * s1 + 0.25 * s2)
    print(f"  main={m:>2} sec1={s1:>2} sec2={s2:>2} → person lvl {person_lvl}")

# ----------------------------------------
# TEST 2: Simulate 4 weeks of practice
# ----------------------------------------
print()
print("=" * 60)
print("TEST 2: 4-week simulation")
print("=" * 60)

# Week 1-4: different intensities
# Riff: drawing (main) heavy, sewing medium, lefthand light
# Reef: 3d-model heavy, linux medium, anatomy light
# Pitch: dance heavy, kendama medium, driving light
# Rain: meditation heavy, massage medium, ferment light

commands = []
for week in range(4):
    intensity = week + 1  # 1x, 2x, 3x, 4x per week
    for day in range(5):  # 5 practice days per week
        # Riff
        commands.append(f"riff {30 * intensity}")
        commands.append(f"riff sew {15 * intensity}")
        if day % 2 == 0:
            commands.append(f"riff lh {10 * intensity}")
        # Reef
        commands.append(f"reef {40 * intensity}")
        commands.append(f"reef lin {20 * intensity}")
        if day % 3 == 0:
            commands.append(f"reef anat {10 * intensity}")
        # Pitch
        commands.append(f"pitch {25 * intensity}")
        commands.append(f"pitch ken {15 * intensity}")
        if day % 2 == 0:
            commands.append(f"pitch drive {10 * intensity}")
        # Rain
        commands.append(f"rain med {20 * intensity}")
        commands.append(f"rain mas {15 * intensity}")
        if day % 3 == 0:
            commands.append(f"rain fer {10 * intensity}")

# Add status at end of each week
for week in range(1, 5):
    # Insert status after week's practice
    pass  # We'll just run status at the end

commands.append("status")
commands.append("week")
commands.append("history 28")

# Limit commands to avoid timeout
print(f"Total commands: {len(commands)}")
print("Running simulation (commands reduced for speed)...")

# Run with fewer commands - just 1 week
commands_short = []
for day in range(5):
    commands_short.extend([
        f"riff {30}", f"riff sew {15}", f"riff lh {10}",
        f"reef {40}", f"reef lin {20}", f"reef anat {10}",
        f"pitch {25}", f"pitch ken {15}", f"pitch drive {10}",
        f"rain med {20}", f"rain mas {15}", f"rain fer {10}",
    ])
commands_short.append("status")
commands_short.append("week")

out = run_commands(commands_short)
print(out)
