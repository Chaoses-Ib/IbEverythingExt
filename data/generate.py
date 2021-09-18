import re
import string

dic = { letter: [] for letter in string.ascii_lowercase }  # { 首字母: [汉字] }

with open('pinyin.txt', encoding='utf8') as f:
    ranges = { letter: [ord(letter), ord(letter)] for letter in string.ascii_lowercase }  # 连续汉字合并成范围

    def append_range(letter, rng):
        length = rng[1] - rng[0]

        # 加入 dic
        lst = dic[letter]
        lst.append(chr(rng[0]))
        if length == 0:  # a
            pass
        elif length == 1:  # ab
            lst.append(chr(rng[1]))
        elif length == 2:  # abc，应该略快于 a-c
            lst.append(chr(rng[0] + 1))
            lst.append(chr(rng[1]))
        else:  # a-d
            lst.append('-')
            lst.append(chr(rng[1]))

    for line in f.readlines()[2:]:
        try:
            hanzi = line[-2]

            # 只保留 GBK 汉字
            hanzi.encode('gbk')

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

            # 处理拼音
            for pinyin in pinyins:
                letter = pinyin[0]
                rng = ranges[letter]
                if ord(hanzi) == rng[1]:  # 忽略同一汉字
                    pass
                elif ord(hanzi) == rng[1] + 1:  # 并入范围
                    rng[1] += 1
                else:  # 开始新 range
                    append_range(letter, rng)
                    rng[:] = [ord(hanzi), ord(hanzi)]
            
        except UnicodeEncodeError:
            pass
    
    # 处理剩余 range
    for letter, rng in ranges.items():
        append_range(letter, rng)

with open('output.txt', 'w', encoding='utf8') as f:
    for key, value in sorted(dic.items()):
        if len(value) == 1:  # i u v
            f.write(f'L"{value[0]}"sv,\n')
        else:
            f.write(f'L"[{"".join(value)}]"sv,\n')