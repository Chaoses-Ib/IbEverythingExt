import string

table = [i for i in range(256)]

for letter in string.ascii_lowercase:
    table[ord(letter)] = ord(letter.upper())

code = ''
for i in range(0, 256, 16):
    code += ', '.join(hex(table[j]) for j in range(i, i+16)) + ',\n'

print(code)