# Real-Time Embedded Voice Recognition System Using STM32 and Edge AI

## Overview

This project implements a real-time embedded audio processing and voice recognition system using an STM32 Cortex-M microcontroller. The system integrates a digital audio codec, real-time audio streaming, DMA-based buffering, and an Edge Impulse machine learning model for low-latency voice command recognition.

The main objective of this project was to develop an efficient embedded AI pipeline capable of capturing audio, processing signals, running an optimized ML inference model, and generating real-time voice command outputs on resource-constrained hardware.

---

## Project Objectives

- Develop a low-latency embedded audio acquisition system.
- Interface and configure an external audio codec using I2C and I2S communication protocols.
- Implement efficient real-time audio streaming using DMA.
- Train and deploy a lightweight machine learning model for voice recognition.
- Optimize ML inference for execution on an STM32 Cortex-M processor.
- Improve system reliability by eliminating audio buffer underruns.

---

# System Architecture
         Audio Input
             |
             v
    +----------------+
    |  WM8960 Codec  |
    +----------------+
          |
          | I2S
          |
          v
    +----------------+
    | STM32 Cortex-M |
    +----------------+
          |
   DMA Ping-Pong Buffer
          |
          v
  Audio Pre-processing
          |
          v
  Feature Extraction
          |
          v
  Command Execution



  
---

# Hardware Components

## Microcontroller

**STM32 Cortex-M MCU**

Responsibilities:

- Audio data acquisition
- Codec communication
- DMA management
- Signal processing
- Machine learning inference


## Audio Codec

**WM8960 Audio Codec**

Features:

- Digital microphone/audio input support
- ADC/DAC conversion
- Configurable sampling rate
- Low-power audio operation

Communication:

- I2C → Codec configuration
- I2S → Real-time audio data transfer

---

# Software Architecture

## 1. Audio Codec Driver Development

A custom WM8960 codec driver was developed to configure and control the audio codec.

Implemented functions include:

- Codec initialization
- Register configuration
- Audio interface setup
- Sample rate configuration
- Volume control
- Power management



---

## Model Optimization

The model was optimized for embedded deployment:

Techniques used:

- Quantization
- Reduced model size
- Lower memory footprint
- Faster inference time

The quantized model enables:

- Integer-based computation
- Reduced RAM usage
- Improved inference speed

---

# Technologies Used

## Embedded

- Embedded C
- STM32 HAL / Low-level drivers
- ARM Cortex-M architecture
- Interrupt handling
- DMA programming

## Communication Protocols

- I2C
- I2S
- SPI (if applicable)

## Audio Processing

- Digital audio streaming
- Sampling rate configuration
- Buffer management
- Signal preprocessing

## Machine Learning

- Edge Impulse
- Neural Networks
- Model quantization
- Embedded inference

---

# Project Structure

