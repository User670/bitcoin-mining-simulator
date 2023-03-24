import re

h=input("Hex string? > ").lower().strip()
assert len(h)%2==0, "Input length not divisible by 2"
assert not re.match("[^0-9a-f]",h), "Input contains characters that is not a hex digit"

t="{\n"
for pos in range(0, len(h), 2):
    t+="0x"
    t+=h[pos]
    t+=h[pos+1]
    if pos!=len(h)-2: t+=", "
    if pos%16==14 and pos!=len(h)-2: t+="\n"
t+="\n}"
print(t)
print(len(h)/2)