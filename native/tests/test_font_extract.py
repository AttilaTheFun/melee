import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('prepare_fonts', Path(__file__).resolve().parents[1] / 'tools/prepare_fonts.py')
fonts = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fonts)

class ExtractionTests(unittest.TestCase):
    def fixture(self):
        data = bytearray(0x180)
        for at, value in [(0, 0x100), (0x48, 0x80001000), (0x90, 0x80)]:
            struct.pack_into('>I', data, at, value)
        data[0x100:] = bytes(range(128))
        return data

    def test_range(self):
        data = self.fixture()
        self.assertEqual(fonts.extract(data, 0x80001010, 16), bytes(range(16, 32)))
        self.assertEqual(fonts.extract(data, 0x80001070, 16), bytes(range(112, 128)))
        with self.assertRaises(ValueError): fonts.extract(data, 0x80001070, 17)

    def test_invalid_sections(self):
        data = self.fixture()
        with self.assertRaises(ValueError): fonts.extract(data[:255], 0x80001000, 16)
        with self.assertRaises(ValueError): fonts.extract(data[:-1], 0x80001000, 16)
        for at, value in [(4, 0x100), (0x4c, 0x80001000), (0x94, 0x80)]:
            struct.pack_into('>I', data, at, value)
        with self.assertRaises(ValueError): fonts.extract(data, 0x80001000, 16)

if __name__ == '__main__': unittest.main()
