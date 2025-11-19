//test.c
#include<stdio.h>

/*
RC4 初始化函数
*/
void rc4_init(unsigned char* s, unsigned char* key, unsigned long Len_k)
{
    int i = 0, j = 0;
    char k[256] = { 0 };
    unsigned char tmp = 0;
    for (i = 0; i < 256; i++) {
        s[i] = i;
        k[i] = key[i % Len_k];
    }
    for (i = 0; i < 256; i++) {
        j = (j + s[i] + k[i]) % 256;
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}

/*
RC4 加解密函数
unsigned char* Data     加解密的数据
unsigned long Len_D     加解密数据的长度
unsigned char* key      密钥
unsigned long Len_k     密钥长度
*/
void RC4(unsigned char* Data, unsigned long Len_D, unsigned char* key, unsigned long Len_k) // 加解密
{
    unsigned char s[256];
    rc4_init(s, key, Len_k);
    int i = 0, j = 0, t = 0;
    unsigned long k = 0;
    unsigned char tmp;
    for (k = 0; k < Len_D; k++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        t = (s[i] + s[j]) % 256;
        Data[k] = Data[k] ^ s[t];
    }
}

void RC4encrypt(unsigned char* Data, unsigned long Len_D, unsigned char* key, unsigned long Len_k) {
    RC4(Data, Len_D, key, Len_k);
}
void RC4decrypt(unsigned char* Data, unsigned long Len_D, unsigned char* key, unsigned long Len_k) {
    RC4(Data, Len_D, key, Len_k);
}
int main()
{
    // 字符串密钥
    unsigned char key[] = "secret";
    unsigned long key_len = sizeof(key) - 1;// 字符串最后还有一个 '/0' 所以需要 - 1
    // 数组密钥
    //unsigned char key[] = {'s','e','c','r','e','t'};
    //unsigned long key_len = sizeof(key);

    unsigned char data[] = { 116, 104, 105, 115, 32, 105, 115, 32, 82, 67, 52, 44, 111, 97, 99, 105, 97 };

    // 对明文进行加密
    RC4encrypt(data, sizeof(data), key, key_len);

    for (int i = 0; i < sizeof(data); i++)
    {
        printf("%d, ", data[i]);
    }
    printf("\n");

    // 对密文进行解密
    RC4encrypt(data, sizeof(data), key, key_len);

    for (int i = 0; i < sizeof(data); i++)
    {
        printf("%c", data[i]);
    }
    printf("\n");
    return 0;
}
/*
153, 94, 187, 111, 162, 205, 165, 134, 96, 136, 143, 240, 156, 135, 150, 94, 204,
this is RC4,oacia
*/