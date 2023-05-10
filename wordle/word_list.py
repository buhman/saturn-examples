import os
import sys

abspath = os.path.abspath(__file__)
dirname = os.path.dirname(abspath)
os.chdir(dirname)

print("namespace wordle {")
print("const uint8_t word_list[][word_length] = {")

answers = list()

with open("word_list.csv", "r") as f:
    index = 0
    for line in f:
        l = line.strip()
        if not l:
            continue
        word, occurrence, day = l.split(",")
        w = word.upper()
        o = eval(occurrence)
        d = day.strip()

        if o > 1e-08:
            print(f"{{'{w[0]}', '{w[1]}', '{w[2]}', '{w[3]}', '{w[4]}'}},")
            if day:
                answers.append(index)
            index+=1

print("};")

print()

print("const uint32_t answers[] = {")
for answer in answers:
    print(f"{answer},")
print("};")

print("}")
print(f"words: {index}", file=sys.stderr)
print(f"answers: {len(answers)}", file=sys.stderr)
