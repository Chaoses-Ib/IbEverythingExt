import re

tables = {
    range(0x3400, 0x9FED+1): [],
    range(0x20000, 0x2CE93+1): [],
    range(0x3007, 0x3007+1): [],
    range(0xE815, 0xE864+1): [],
    range(0x30EDD, 0x30EDE+1): []
}
for rng, lst in tables.items():
    lst[:] = [0] * (rng.stop - rng.start)

with open('pinyin.txt', encoding='utf8') as f:
    for line in f.readlines()[2:]:
        # 获取拼音
        begin = line.find(': ') + 2
        pinyin_seq = line[begin:-6]
        # (?:: |,)[^a-zāáǎàēéěèê̄ếê̌ềōóǒòḿńňǹ]
        pinyin_seq = re.sub('[āáǎà]', 'a', pinyin_seq)
        pinyin_seq = re.sub('[ēéěèê̄ếê̌ề]', 'e', pinyin_seq)
        pinyin_seq = re.sub('[ōóǒò]', 'o', pinyin_seq)
        pinyin_seq = re.sub('[ḿ]', 'm', pinyin_seq)
        pinyin_seq = re.sub('[ńňǹ]', 'n', pinyin_seq)
        pinyins = pinyin_seq.split(',')

        # 转换成 flags
        pinyin_flags = 0
        for pinyin in pinyins:
            pinyin_flags |= 2 ** (ord(pinyin[0]) - ord('a'))

        # 保存到 tables
        hanzi = ord(line[-2])
        for rng, lst in tables.items():
            if hanzi in rng:
                lst[hanzi - rng.start] = pinyin_flags
                break
        else:
            raise

with open(f'output_ord_pinyin.txt', 'w', encoding='utf8') as f:
    for rng, lst in tables.items():
        f.write('uint32_t table_{:X}_{:X}[]{{ {} }};\n'.format(
            rng.start,
            rng.stop - 1,
            ','.join(str(flags) for flags in lst)  # 16^n ≥ 10^(n+2) -> n ≥ 10，十位以下用 0x十六进制 的编码效率低于十进制
            ))
    
    f.write(f'''
PinyinRange pinyin_ranges[{ len(tables) }]{{
''')

    for rng, lst in tables.items():
        f.write('{{ 0x{0:X}, 0x{1:X}, table_{0:X}_{1:X} }},\n'.format(
            rng.start,
            rng.stop - 1
            ))
    
    f.write('};')