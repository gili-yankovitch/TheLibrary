#!/usr/bin/python3

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
from binascii import hexlify
import os

AES_BLOCKLEN = 16

class Stage:
    FLAG_BANNER = "MAGICLIB"

    def __init__(self, flag):
        self.flag = flag

        self.setup()

    def setup(self):
        pass

    def solve(self):
        pass

class Stage1(Stage):
    def __init__(self):
        Stage.__init__(self, Stage.FLAG_BANNER + "{No one can break this! 0x20000}")

    def setup(self):
        self.xor_key = [0x77, 0x21, 0x0f, 0x53, 0x21, 0x7f, 0xef, 0x98]

        self.flag = "".join([ chr(ord(c) ^ x) for c, x in zip(self.flag, self.xor_key * (len(self.flag) // len(self.xor_key) + 1))])

        print(hexlify(bytes(self.flag[:4], "ascii")).decode("utf-8"))

        # Mess up the first four bytes
        self.flag = "\x00\x00\x00\x00" + self.flag[4:]

    def solve(self):
        known = Stage.FLAG_BANNER[4:] + "{No "

        key = bytes([ ord(x) ^ ord(b) for x, b in zip(known, self.flag[4:4 + len(known)]) ])
        key = key[4:] + key[:4]

        message = "".join([ chr(ord(c) ^ x) for c, x in zip(self.flag, key * (len(self.flag) // len(key) + 1))])

        # Fix the first four letters:
        fixed = bytes([ k ^ ord(m) for k, m in zip(key[:4], Stage.FLAG_BANNER[:4])])
        print(hexlify(fixed).decode('utf-8'))

        print(message)

class Stage2(Stage):
    def __init__(self):
        Stage.__init__(self, "Important message to transmit - " + Stage.FLAG_BANNER + "{53Cr37 5745H: 0x30000}")

    def setup(self):
        self.aes_key = bytes([0x0d, 0x70, 0xb8, 0x05, 0xed, 0xeb, 0x72, 0x3a, 0x5c, 0xcd, 0x12, 0x23, 0xb9, 0x34, 0x62, 0x1c])

        padder = padding.PKCS7(128).padder()

        padded_data = padder.update(bytes(self.flag, "ascii")) + padder.finalize()

        encryptor = Cipher(algorithms.AES(self.aes_key), modes.ECB(), backend=default_backend()).encryptor()

        self.flag = encryptor.update(padded_data) + encryptor.finalize()

        print(padded_data)

    def solve(self):
        decryptor = Cipher(algorithms.AES(self.aes_key), modes.ECB(), backend=default_backend()).decryptor()

        # First, decrypt normally
        decrypted_padded_data = decryptor.update(self.flag) + decryptor.finalize()

        unpadder = padding.PKCS7(128).unpadder()

        ecb_decrypted_data = unpadder.update(decrypted_padded_data) + unpadder.finalize()

        # Swap blocks to decrypt what actually matters
        print(ecb_decrypted_data[:AES_BLOCKLEN * 2])

        flag = self.flag[AES_BLOCKLEN * 2:]

        decryptor = Cipher(algorithms.AES(self.aes_key), modes.ECB(), backend=default_backend()).decryptor()

        decrypted_padded_data = decryptor.update(flag) + decryptor.finalize()

        unpadder = padding.PKCS7(128).unpadder()

        ecb_decrypted_data = unpadder.update(decrypted_padded_data) + unpadder.finalize()

        print(ecb_decrypted_data[:AES_BLOCKLEN * 2])

class Stage3(Stage):
    def __init__(self):
        Stage.__init__(self, Stage.FLAG_BANNER + "{Passwd: 4 53Cr37 P455}")

    def setup(self):
        self.aes_key = bytes([0x0d, 0x70, 0xb8, 0x05, 0xed, 0xeb, 0x72, 0x3a, 0x5c, 0xcd, 0x12, 0x23, 0xb9, 0x34, 0x62, 0x1c])

        iv = bytes([ 0 ] * AES_BLOCKLEN)

        encryptor = Cipher(algorithms.AES(self.aes_key), modes.CBC(iv), backend=default_backend()).encryptor()

        # Pad
        padder = padding.PKCS7(128).padder()
        padded_data = padder.update(bytes(self.flag, "ascii")) + padder.finalize()

        self.flag = encryptor.update(padded_data) + encryptor.finalize()

    def solve(self):
        plaintext = []
        decbytes = []

        flag = bytearray(self.flag)

        # How many blocks?
        blocks = len(flag) // AES_BLOCKLEN

        # Perform a padding-oracle attack
        for i in range(1, AES_BLOCKLEN):
            # print(flag[AES_BLOCKLEN * (blocks - 1) - i], AES_BLOCKLEN * (blocks - 1) - i)

            for c in range(256):
                # Change the LAST byte of the n-1th block
                #print(f"Breaking idx: {AES_BLOCKLEN * (blocks - 1) - i}")
                flag[AES_BLOCKLEN * (blocks - 1) - i] = c

                iv = bytes([ 0 ] * AES_BLOCKLEN)
                decryptor = Cipher(algorithms.AES(self.aes_key), modes.CBC(iv), backend=default_backend()).decryptor()
                decrypted_padded_data = decryptor.update(flag) + decryptor.finalize()
                unpadder = padding.PKCS7(128).unpadder()

                try:
                    cbc_decrypted_data = unpadder.update(decrypted_padded_data) + unpadder.finalize()
                except:
                    continue

                # print(cbc_decrypted_data)
                print(f"Found correct padding for pad ( = {i}) ^ c[{AES_BLOCKLEN * (blocks - 1) - i}] (= {c}) = {i ^ c ^ self.flag[AES_BLOCKLEN * (blocks - 1) - i]}")
                decbytes = [i ^ c] + decbytes
                plaintext = [decbytes[0] ^ self.flag[AES_BLOCKLEN * (blocks - 1) - i]] + plaintext

                # Prepare for the next iteration
                for j in range(len(decbytes)):
                    # print(f"\tSetting new padding byte: {i + 1}: {decbytes[len(decbytes) - j - 1]}")
                    flag[AES_BLOCKLEN * (blocks - 1) - j - 1] = decbytes[len(decbytes) - j - 1] ^ (i + 1)
                break

        p = "".join([chr(c) for c in plaintext])
        print(f"'{p}'")

if __name__ == "__main__":
    stage1 = Stage1()
    stage1.solve()

    stage2 = Stage2()
    stage2.solve()

    stage3 = Stage3()
    stage3.solve()

    exit(0)
# AES key must be 32 bytes for AES-256
key = os.urandom(32)
plaintext = b"This is a secret message."

# Padding the plaintext for AES block size (16 bytes)
padder = padding.PKCS7(128).padder()
padded_data = padder.update(plaintext) + padder.finalize()

# ECB Mode Example
ecb_cipher = Cipher(algorithms.AES(key), modes.ECB(), backend=default_backend())

decryptor = ecb_cipher.decryptor()
decrypted_padded_data = decryptor.update(ecb_ciphertext) + decryptor.finalize()
unpadder = padding.PKCS7(128).unpadder()
ecb_decrypted_data = unpadder.update(decrypted_padded_data) + unpadder.finalize()

print("ECB Encrypted:", ecb_ciphertext)
print("ECB Decrypted:", ecb_decrypted_data)

# CBC Mode Example
iv = os.urandom(16)  # 16 bytes for AES block size
cbc_cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
encryptor = cbc_cipher.encryptor()
cbc_ciphertext = encryptor.update(padded_data) + encryptor.finalize()

decryptor = cbc_cipher.decryptor()
decrypted_padded_data = decryptor.update(cbc_ciphertext) + decryptor.finalize()
unpadder = padding.PKCS7(128).unpadder()
cbc_decrypted_data = unpadder.update(decrypted_padded_data) + unpadder.finalize()

print("CBC Encrypted:", cbc_ciphertext)
print("CBC Decrypted:", cbc_decrypted_data)

