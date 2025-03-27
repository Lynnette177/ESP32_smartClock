#pragma once
#include "includes.h"
bool encode(const char *account, const char *password, char *key, char *output)
{
    char *plain_text = new char[1024];
    strcpy(plain_text, account);
    strcat(plain_text, "%%%");
    strcat(plain_text, password);
    strcpy(output, plain_text);
    // Serial.println(plain_text);
    char *sharp = strchr(key, '#');
    if (sharp == NULL)
    {
        return false;
    }
    sharp++;
    // Serial.println(sharp);

    int plain_index = 0;
    char *encoded_ptr = output;
    while (*sharp != 0 && plain_index < strlen(plain_text))
    {                                             // 避免二者之一越界
        int insert_num = *sharp - 48;             // 用ASCII把这个转换成数字。这是明文每个字符插入多少个
        *encoded_ptr = plain_text[plain_index++]; // 先插入一个明文字符
        if (*encoded_ptr == '%')
        {
            encoded_ptr++;
            *encoded_ptr = '2';
            encoded_ptr++;
            *encoded_ptr = '5';
        }
        encoded_ptr++;
        for (int i = 0; i < insert_num; i++)
        {
            *encoded_ptr = *key;
            key++;
            encoded_ptr++; // 指针后移
        }
        sharp++; // 指针后移
    }
    if (*sharp == 0 && plain_index < strlen(plain_text))
    { // 如果密钥用完了明文还有剩余
        while (plain_index < strlen(plain_text))
        { // 插入剩余全部明文字符
            *encoded_ptr = plain_text[plain_index++];
            encoded_ptr++;
        }
    }
    else if (*sharp != 0 && !plain_index < strlen(plain_text))
    { // 如果明文用完了密钥还有剩余
        while (*key != '#')
        {
            *encoded_ptr = *key;
            encoded_ptr++;
            key++;
        }
    }
    *encoded_ptr = 0;
    // Serial.println(output);
    delete[] plain_text;
    return true;
}
void encrypt(const unsigned char *input, size_t input_len, const unsigned char *key, unsigned char *output, size_t *output_len)
{
    // AES加密上下文
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // 设置AES加密密钥
    mbedtls_aes_setkey_enc(&aes, key, 128);

    // 填充输入数据到AES块大小
    size_t padded_len = ((input_len / 16) + 1) * 16;
    unsigned char padded_input[padded_len];
    memcpy(padded_input, input, input_len);
    for (int i = input_len; i < padded_len; i++)
    {
        padded_input[i] = padded_len - input_len;
    }

    // AES加密
    for (size_t i = 0; i < padded_len; i += 16)
    {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, padded_input + i, output + i);
    }

    // 计算加密后的数据长度
    *output_len = padded_len;

    // 释放AES上下文
    mbedtls_aes_free(&aes);
}
void base64_encode(const unsigned char *input, size_t input_len, unsigned char *output, size_t *output_len)
{
    mbedtls_base64_encode(output, *output_len, output_len, input, input_len);
}
void do_encrypt(const unsigned char *key, char *password, char *output)
{
    char *t = new char[strlen(password) + 10];
    strcpy(t, "\"");
    strcat(t, password);
    strcat(t, "\"");
    size_t t_len = strlen(t); // 排除结尾的空字符

    // 加密后的数据缓冲区
    unsigned char encrypted_data[32];
    size_t encrypted_len;

    // 加密数据
    encrypt((unsigned char *)t, t_len, key, encrypted_data, &encrypted_len);

    // Base64编码后的数据缓冲区
    unsigned char encrypted_data_base64[64];
    size_t encrypted_base64_len = sizeof(encrypted_data_base64);

    // Base64编码加密后的数据
    base64_encode(encrypted_data, encrypted_len, encrypted_data_base64, &encrypted_base64_len);

    // 输出Base64编码后的加密数据
    Serial.printf("Encrypted data (Base64): %.*s\n", (int)encrypted_base64_len, encrypted_data_base64);
    delete[] t;
    size_t output_len;
    base64_encode(encrypted_data_base64, encrypted_base64_len, (unsigned char *)output, &output_len);
}