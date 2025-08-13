import sys

with open(sys.argv[1]) as f:
    data = list(map(int, f.read().strip().split()))

for a in data:
    for b in data:
        if a + b == 2020:
            print(a, b, a * b)
