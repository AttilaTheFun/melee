#!/usr/bin/env python3
"""Translate the repository's unmodified PPC reverb loop into a test-only oracle.

This deliberately retains registers, byte offsets, branch labels and instruction
order. It is neither linked into the native game nor a runtime PPC backend.
"""
from pathlib import Path
import re
import sys
root = Path(__file__).resolve().parents[2]
kind = sys.argv[2] if len(sys.argv) > 2 else 'std'
assert kind in ('std', 'hi', 'cross', 'src1', 'src2')
source_name = 'chorus.c' if kind.startswith('src') else 'reverb_' + ('std' if kind == 'std' else 'hi') + '.c'
source = (root / 'extern/dolphin/src/dolphin/axfx' / source_name).read_text()
function = 'do_' + kind if kind.startswith('src') else 'DoCrossTalk' if kind == 'cross' else 'HandleReverb'
body = source[source.index('asm static void ' + function):]
start, end = {'std': ('lis r31, value0_3', 'lfd f14, 88(r1)'),
              'hi': ('stw k, 0x50(r1)', 'lfd f14, 0x60(r1)'),
              'cross': ('lis r5, i2fMagic', 'lfd f14, 40(r1)'),
              'src1': ('lwz r4,', 'lmw r26,'), 'src2': ('lwz r4,', 'lmw r26,')}[kind]
body = body[body.index(start):body.index(end)]
fields = {'allPassCoeff': 0xf0, 'damping': 0x11c, 'level': 0x118,
          'combCoef[0]': 0xf4, 'combCoef[1]': 0xf8, 'lpLastout[0]': 0x10c,
          'lpLastout': 0x10c, 'preDelayLine[0]': 0x124,
          'preDelayPtr[0]': 0x130, 'preDelayPtr': 0x130, 'preDelayTime': 0x120,
          'C': 0x78, 'AP': 0}
if kind == 'hi':
    fields = {'allPassCoeff': 0x168, 'damping': 0x1a0, 'level': 0x19c,
              'combCoef[0]': 0x16c, 'combCoef[1]': 0x170, 'combCoef[2]': 0x174,
              'lpLastout[0]': 0x190, 'lpLastout': 0x190,
              'preDelayLine': 0x1ac, 'preDelayPtr': 0x1b8, 'preDelayTime': 0x1a4,
              'C': 0xb4, 'AP': 0}
line_fields = {'inPoint': 0, 'outPoint': 4, 'length': 8, 'inputs': 12, 'lastOutput': 16}
def reg(value):
    return {'rv': 'r[4]', 'sptr': 'r[3]', 'k': 'r[5]', 'l': 'r[3]', 'r': 'r[4]', 'src': 'r[3]', 'cross': 'f[1]', 'invcross': 'f[2]'}.get(value, re.sub(r'^([rf])(\d+)$', r'\1[\2]', value))
def expr(value):
    if kind.startswith('src'):
        for name, offset in {'dest':0,'smpBase':4,'old':8,'posLo':12,'posHi':16,'pitchLo':20,'pitchHi':24,'trigger':28,'target':32}.items():
            value = value.replace('AXFX_CHORUS_SRCINFO.' + name, str(offset))
    for name, offset in sorted(fields.items(), key=lambda x: -len(x[0])):
        value = value.replace(('AXFX_REVHI_WORK.' if kind == 'hi' else 'AXFX_REVSTD_WORK.') + name, str(offset))
    for name, offset in line_fields.items():
        value = value.replace(('AXFX_REVHI_DELAYLINE.' if kind == 'hi' else 'AXFX_REVSTD_DELAYLINE.') + name, str(offset))
    return reg(value)
def address(value):
    match = re.fullmatch(r'(.*?)\((\w+)\)', value)
    assert match, value
    return f'({reg(match[2])} + ({expr(match[1])}))'
output = ['/* Generated from SDK assembly by generate_reverb_reference.py. */',
          'static void reference_process(void) {',
          'uint32_t r[32] = {0}; double f[32] = {0}; int cr[8] = {0}; unsigned ctr = 0;',
          'r[1] = 0x300000; r[3] = 0x2000; r[4] = 0x1000;']
if kind == 'hi':
    output[1] = 'static void reference_hi(unsigned channel) {'
    output[-1] = 'r[1] = 0x300000; r[3] = 0x2000 + channel * 640; r[4] = 0x1000; r[5] = channel;'
elif kind == 'cross':
    output[1] = 'static void reference_cross(float cross, float inverse) {'
    output[-1] = 'r[1] = 0x300000; r[3] = 0x2000; r[4] = 0x2280; f[1] = cross; f[2] = inverse; double p[32] = {0};'
if kind.startswith('src'):
    output[1] = 'static void reference_' + kind + '(void) {'
    output[-1] = 'r[1] = 0x300000; r[3] = 0x1000; unsigned carry = 0;'
for raw in body.splitlines():
    line = raw.split('//')[0].strip()
    if not line: continue
    if line.endswith(':'):
        output.append(line + ' ;')
        continue
    op, operands = line.split(None, 1)
    args = [x.strip() for x in operands.split(',')]
    a = [expr(x) for x in args]
    if op == 'lis':
        code = f'{a[0]} = 0;' if '@ha' in args[1] else f'{a[0]} = (uint32_t)({a[1]}) << 16;'
    elif op == 'addi' and 'rsmpTab12khz@l' in args[2]:
        code = f'{a[0]} = 0x4000;'
    elif op in ('lfs', 'lfd') and '@l' in args[1]:
        constant = {'value0_3': '0.3f', 'value0_6': '0.6f', 'i2fMagic': '4503601774854144.0'}[args[1].split('@')[0]]
        code = f'{a[0]} = {constant};'
    elif op in ('lfs', 'lfd', 'lwz'):
        fn = {'lfs': 'ref_float', 'lfd': 'ref_double', 'lwz': 'ref_word'}[op]
        code = f'{a[0]} = {fn}({address(args[1])});'
    elif op in ('stw', 'stfs'):
        fn = 'ref_put_word' if op == 'stw' else 'ref_put_float'
        code = f'{fn}({address(args[1])}, {a[0]});'
    elif op == 'lwzu':
        base = reg(re.search(r'\((\w+)\)', args[1])[1])
        code = f'{base} = {address(args[1])}; {a[0]} = ref_word({base});'
    elif op == 'lwzx': code = f'{a[0]} = ref_word({a[1]} + {a[2]});'
    elif op in ('stfsx', 'stfiwx', 'lfsx'):
        addr = f'({a[1]} + {a[2]})'
        code = f'{a[0]} = ref_float({addr});' if op == 'lfsx' else f'{"ref_put_float" if op == "stfsx" else "ref_put_word"}({addr}, {a[0]});'
    elif op in ('fmadds', 'fnmsubs'):
        value = f'fmaf((float){a[1]}, (float){a[2]}, {"-" if op == "fnmsubs" else ""}(float){a[3]})'
        code = f'{a[0]} = {"-" if op == "fnmsubs" else ""}{value};'
    elif op in ('fmuls', 'fsubs', 'fadds'):
        symbol = {'fmuls': '*', 'fsubs': '-', 'fadds': '+'}[op]
        code = f'{a[0]} = (float)({a[1]} {symbol} {a[2]});'
    elif op == 'fctiwz': code = f'{a[0]} = ref_integer({a[1]});'
    elif op == 'fctiw': code = f'{a[0]} = ref_integer(nearbyint({a[1]}));'
    elif op == 'ps_merge00':
        code = f'{a[0].replace("f[", "p[")} = {a[2]}; {a[0]} = {a[1]};'
    elif op in ('ps_muls0', 'ps_mul'):
        p = [x.replace('f[', 'p[') for x in a]
        code = f'{p[0]} = (float)({p[1]} * {a[2] if op == "ps_muls0" else p[2]}); {a[0]} = (float)({a[1]} * {a[2]});'
    elif op == 'ps_sum0':
        code = f'{a[0]} = (float)({a[1]} + {a[2].replace("f[", "p[")}); {a[0].replace("f[", "p[")} = {a[3].replace("f[", "p[")};'
    elif op in ('mr', 'fmr', 'li'): code = f'{a[0]} = {a[1]};'
    elif op in ('addi', 'add', 'subi', 'mulli', 'slwi'):
        symbol = {'addi': '+', 'add': '+', 'subi': '-', 'mulli': '*', 'slwi': '<<'}[op]
        code = f'{a[0]} = {a[1]} {symbol} ({a[2]});'
    elif op == 'xoris': code = f'{a[0]} = {a[1]} ^ ((uint32_t)({a[2]}) << 16);'
    elif op in ('cmpw', 'cmpwi'):
        condition = int(args.pop(0)[2:]) if args[0].startswith('cr') else 0
        code = f'cr[{condition}] = ({expr(args[0])} == {expr(args[1])});'
    elif op.rstrip('+') in ('bne', 'beq'):
        condition = int(args[0][2:]) if len(args) == 2 else 0
        code = f'if ({"!" if op.startswith("bne") else ""}cr[{condition}]) goto {args[-1]};'
    elif op == 'rlwinm':
        shift, first, last = map(int, args[2:])
        mask = sum(1 << (31-i) for i in range(first, last+1))
        code = f'{a[0]} = (({a[1]} << {shift}) | ({a[1]} >> {32-shift})) & {mask}u;'
    elif op == 'addc':
        code = f'{{ uint64_t sum = (uint64_t){a[1]} + {a[2]}; carry = sum >> 32; {a[0]} = (uint32_t)sum; }}'
    elif op == 'mcrxr': code = f'cr[{int(args[0][2:])}] = carry; carry = 0;'
    elif op == 'b': code = f'goto {args[0]};'
    elif op == 'bdz': code = f'if (!--ctr) goto {args[0]};'
    elif op == 'mtctr': code = f'ctr = {a[0]};'
    elif op == 'bdnz': code = f'if (--ctr) goto {args[0]};'
    else: raise ValueError(line)
    if op == 'stfiwx':
        code = code.replace(', ' + a[0] + ');', ', (uint32_t)(int32_t)' + a[0] + ');')
    output.append(code + ' /* ' + line + ' */')
output.append('}')
Path(sys.argv[1]).write_text('\n'.join(output) + '\n')
