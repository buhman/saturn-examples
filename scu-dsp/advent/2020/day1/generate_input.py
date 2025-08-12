import random

def has_2020_2(l):
    length = len(l)
    found = set()
    for i in range(length):
        for j in range(length):
            if i == j:
                continue
            if l[i] + l[j] == 2020:
                found.add(frozenset((i, j)))
    return len(found) == 1

def has_2020_3(l):
    length = len(l)
    found = set()
    for i in range(length):
        for j in range(length):
            for k in range(length):
                if i == j or k == i:
                    continue
                if l[i] + l[j] + l[k] == 2020:
                    found.add(frozenset((i, j)))
    return len(found) == 1

def gen_random():
    return [random.randint(100, 2500) for _ in range(64)]

def find_input():
    while True:
        l = gen_random()
        if has_2020_2(l) and has_2020_3(l):
            return l

print("\n".join(map(str, find_input())))
