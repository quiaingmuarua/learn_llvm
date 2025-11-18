# toy_feistel.py
# 教学用 Feistel 分组密码（仅用于学习，非生产级安全）

from typing import List
import struct

# ---- 参数 ----
BLOCK_SIZE_BITS = 64
HALF_SIZE = BLOCK_SIZE_BITS // 2  # 32
ROUNDS = 8
KEY_SIZE_BITS = 128

# ---- 简单 S-box: 4-bit 输入 -> 4-bit 输出 （教学用） ----
SBOX = [
    0xE, 0x4, 0xD, 0x1,
    0x2, 0xF, 0xB, 0x8,
    0x3, 0xA, 0x6, 0xC,
    0x5, 0x9, 0x0, 0x7
]
# 逆 S-box
INV_SBOX = [SBOX.index(x) for x in range(16)]

def apply_sbox_32(x: int) -> int:
    """对 32-bit 输入按 4-bit 块应用 S-box，返回 32-bit 输出"""
    out = 0
    for i in range(8):  # 32 / 4 = 8 个 4-bit 片
        nibble = (x >> (4 * i)) & 0xF
        out |= (SBOX[nibble] << (4 * i))
    return out & 0xFFFFFFFF

def apply_inv_sbox_32(x: int) -> int:
    out = 0
    for i in range(8):
        nibble = (x >> (4 * i)) & 0xF
        out |= (INV_SBOX[nibble] << (4 * i))
    return out & 0xFFFFFFFF

# ---- 简单 key schedule（教学用） ----
def expand_key(master_key_bytes: bytes) -> List[int]:
    """把 128-bit 主密钥（16 字节）扩展为 ROUNDS 个 32-bit 子密钥"""
    if len(master_key_bytes) != 16:
        raise ValueError("master key must be 16 bytes (128 bits)")
    # 把主密钥分成四个 32-bit words，然后轮移并混合生成子密钥
    words = list(struct.unpack(">4I", master_key_bytes))  # 大端
    subkeys = []
    for r in range(ROUNDS):
        # 一个非常简单的调度：对 words 做循环移位和加常数（教学用）
        idx = r % 4
        k = words[idx] ^ ((r * 0x9e3779b9) & 0xFFFFFFFF)  # 0x9e3779b9 是黄金分割常数
        subkeys.append(k)
        # 让 words 演化
        words = words[1:] + [ ((words[0] ^ k) << 1) & 0xFFFFFFFF | ((words[0] ^ k) >> 31) ]
    return subkeys

# ---- 轮函数 F ----
def round_function(right: int, subkey: int) -> int:
    """教学用轮函数：
       - 先与子密钥异或
       - 应用 S-box（按 4-bit 块）
       - 循环左移 7 位
       - 再与子key相加（模 2^32）
    """
    x = right ^ subkey
    x = apply_sbox_32(x)
    x = ((x << 7) & 0xFFFFFFFF) | (x >> (32 - 7))
    x = (x + subkey) & 0xFFFFFFFF
    return x

# ---- Feistel 加解密单块 ----
def encrypt_block(block8: bytes, subkeys: List[int]) -> bytes:
    if len(block8) != 8:
        raise ValueError("block must be 8 bytes")
    L, R = struct.unpack(">2I", block8)  # 32-bit each
    for r in range(ROUNDS):
        F = round_function(R, subkeys[r])
        L, R = R, L ^ F
    # 在 Feistel 结构中最后一轮交换回加密输出通常是 (R, L)
    return struct.pack(">2I", L, R)

def decrypt_block(block8: bytes, subkeys: List[int]) -> bytes:
    if len(block8) != 8:
        raise ValueError("block must be 8 bytes")
    L, R = struct.unpack(">2I", block8)
    # 逆向轮次
    for r in reversed(range(ROUNDS)):
        # 反向 Feistel: 之前的 L,R = R_prev, L_prev ^ F(R_prev)
        # 要恢复 L_prev,R_prev：
        # L_prev = R
        # R_prev = L ^ F(R_prev)  <-- 但 F 只依赖于 R_prev，所以我们需要用 R_prev variable trick:
        # Alternate view: since encrypt step was (L,R)->(R, L^F(R)), decrypt does:
        R, L = L, R ^ round_function(L, subkeys[r])
    return struct.pack(">2I", L, R)

# ---- Helper: hex display ----
def bytes_to_hex(b: bytes) -> str:
    return b.hex()

# ---- 简单测试 ----
if __name__ == "__main__":
    # 测试：加密 -> 解密 应恢复原文
    key = b"\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10"  # 16 bytes
    subkeys = expand_key(key)

    plaintext = b"ABCDEFGH"  # 8 bytes
    ct = encrypt_block(plaintext, subkeys)
    pt = decrypt_block(ct, subkeys)

    print("Key       :", key.hex())
    print("Plaintext :", plaintext, plaintext.hex())
    print("Ciphertext:", ct.hex())
    print("Decrypted :", pt, pt.hex())
    assert pt == plaintext, "decrypt failed!"

    # Avalanche test: flip one bit in plaintext
    flip = bytearray(plaintext)
    flip[0] ^= 0x01
    ct2 = encrypt_block(bytes(flip), subkeys)
    print("\nAvalanche test:")
    print("PT1:", plaintext.hex())
    print("PT2:", bytes(flip).hex())
    print("CT1:", ct.hex())
    print("CT2:", ct2.hex())
    # 计算不同的比特数
    def bit_diff(a: bytes, b: bytes) -> int:
        x = int.from_bytes(a, 'big') ^ int.from_bytes(b, 'big')
        return x.bit_count()
    print("Cipher bit differences:", bit_diff(ct, ct2))
