#pragma once
#include "includes.h"
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK 4          // sck26
#define I2S_MIC_LEFT_RIGHT_CLOCK 5      // ws22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_19 // sd34
#define SAMPLE_RATE (44100)

#define speaker_BCLK GPIO_NUM_38              //
#define speaker_LRCK_ws GPIO_NUM_39           // 引脚号
#define speaker_DATA_out GPIO_NUM_37          // 引脚号
#define speaker_data_in_num I2S_PIN_NO_CHANGE // DATA_IN引脚号

// 不要改任何东西
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
    .use_apll = false,
   };

// 不要改任何东西
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};
// 以下是扬声器的
i2s_config_t i2s_config_mx = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // 使用主模式并设置为发送数据
    .sample_rate = 44100,                                // 设置采样率为44100Hz
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,        // 设置每个采样点的位数为16位
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,         // 只使用右声道
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // I2S通信格式
    .intr_alloc_flags = 0,                               // 分配中断标志
    .dma_buf_count = 8,                                  // 设置DMA缓冲区数量为8
    .dma_buf_len = 1024,                                 // 每个DMA缓冲区的长度为1024字节
};
i2s_pin_config_t pin_speaker_config = {
    .bck_io_num = speaker_BCLK,         // BCLK引脚号
    .ws_io_num = speaker_LRCK_ws,       // LRCK引脚号
    .data_out_num = speaker_DATA_out,   // DATA引脚号
    .data_in_num = speaker_data_in_num, // DATA_IN引脚号
};
int I2S_Read(void *data, int numData)
{
    size_t bytesRead;
    i2s_read(I2S_NUM_0, data, numData, &bytesRead, 1000 / portTICK_PERIOD_MS);
    return bytesRead;
}
int I2S_Write(void *data, int numData)
{
    size_t bytesRead;
    return i2s_write(I2S_NUM_1, data, numData, &bytesRead, 1000 / portTICK_PERIOD_MS);
}

void read_mic_to_buffer(char * DataBuffer_out){
    int numCommunicationData = 1024;
    char * communicationData = new char[numCommunicationData];
    char* partWavData = new char[256];
    I2S_Read(communicationData, numCommunicationData);
    for (int i = 0; i < numCommunicationData/8; ++i) {
      partWavData[2*i] = communicationData[8*i + 2];
      partWavData[2*i + 1] = communicationData[8*i + 3];
    }
    memcpy(DataBuffer_out,partWavData, 256);
    delete[] partWavData;
    delete[] communicationData;
}