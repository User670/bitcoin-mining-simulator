f_names=[
    "void_star",
    "char_star",
    "unsigned_char_star",
    "char_array",
    "unsigned_char_array",
    "const_char_star",
    "const_unsigned_char_star",
    "typedef_struct"
]

identifiers=[
    "p_void_star",
    "p_char_star",
    "p_unsigned_char_star",
    "p_char_array",
    "p_unsigned_char_array",
    "p_const_char_star",
    "p_const_unsigned_char_star",
    "p_char_array_eq_literal",
    "p_char_star_eq_literal",
    '"literal_string"',
    "p_typedef_struct"
]

r=[]
with open("used_lines.txt") as f:
    r=f.read().split("\n")

t=""
for f in f_names:
    for i in identifiers:
        this=f"{f}({i});"
        if this not in r:
            t+=this+"\n"
            r.append(this)
    t+="\n"

with open("used_lines.txt","w") as f:
    f.write("\n".join(r))

print(t)
