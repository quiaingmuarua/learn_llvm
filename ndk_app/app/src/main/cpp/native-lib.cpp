#include <jni.h>
#include <string>
#include <cstdint>

// Simple XOR cipher — gives obfuscation passes meaningful arithmetic to work with
static void xor_cipher(uint8_t *data, size_t len, uint8_t key) {
    uint8_t k = key;
    for (size_t i = 0; i < len; i++) {
        k = (k * 6364136223846793005ULL + 1442695040888963407ULL) & 0xFF;
        data[i] ^= k;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ndk_1example_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    char msg[] = {0x49, 0x6c, 0x6c, 0x6f, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x43, 0x2b, 0x2b, 0};
    xor_cipher(reinterpret_cast<uint8_t*>(msg), sizeof(msg) - 1, 0x42);
    return env->NewStringUTF(msg);
}
