#include "wm8960.h"
#include <stdio.h>

HAL_StatusTypeDef WM8960_WriteReg(WM8960_Handle_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (reg << 1) | ((value >> 8) & 0x01);
    data[1] = value & 0xFF;
    return HAL_I2C_Master_Transmit(dev->hi2c, WM8960_I2C_ADDR, data, 2, 100);
}

/* ============================================================================
 * RESET
 * ============================================================================ */
HAL_StatusTypeDef WM8960_Reset(WM8960_Handle_t *dev)
{
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_RESET, 0x000);
    HAL_Delay(10);
    return ret;
}

/* ============================================================================
 * INIT — Full codec initialisation
 * Brings up clocking, power, audio format, DAC, ADC, MIC, outputs
 * ============================================================================ */
HAL_StatusTypeDef WM8960_Init(WM8960_Handle_t *dev, I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;

    dev->hi2c       = hi2c;
    dev->dac_muted  = 0;
    dev->adc_muted  = 0;
    dev->output      = WM8960_OUTPUT_BOTH;
    dev->input       = WM8960_INPUT_MIC1;

    /* Reset */
    printf("WM8960 Init starting\r\n");
    ret = WM8960_Reset(dev);
    if (ret != HAL_OK) return ret;

    /* Clocking 1: SYSCLK = MCLK, no PLL, no divider */

    ret = WM8960_WriteReg(dev, WM8960_REG_CLOCKING1, 0x000);
    printf("Clocking1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* Clocking 2: DCLKDIV = SYSCLK/16 */
    ret = WM8960_WriteReg(dev, WM8960_REG_CLOCKING2, 0x1C0);
    printf("Clocking2: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* Power 1: VMIDSEL=01, VREF=1, AINL=1, AINR=1, ADCL=1, ADCR=1, MICB=1 */
    ret = WM8960_WriteReg(dev, WM8960_REG_POWER1, 0x0FE);
    printf("Power1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* Power 2: DACL=1, DACR=1, LOUT1=1, ROUT1=1 */
    ret = WM8960_WriteReg(dev, WM8960_REG_POWER2, 0x1E0);
    printf("Power2: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* Power 3: LOMIX=1, ROMIX=1, LMIC=1, RMIC=1 */
    ret = WM8960_WriteReg(dev, WM8960_REG_POWER3, 0x03C);
    printf("Power3: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* Audio Interface: I2S, 16-bit, SLAVE mode */
    ret = WM8960_WriteReg(dev, WM8960_REG_AUDIO_IFACE, 0x002);
    printf("Format: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* DAC: unmute, no deemphasis */
    ret = WM8960_WriteReg(dev, WM8960_REG_ADCDAC, 0x000);
    printf("DAC unmute: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* DAC volume: max (0dB) with update bit */
    ret = WM8960_WriteReg(dev, WM8960_REG_DACVOL1, 0x1FF);
    printf("LDAC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_DACVOL2, 0x1FF);
    printf("RDAC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* ADC volume: 0dB with update bit */
    ret = WM8960_WriteReg(dev, WM8960_REG_ADCVOLL, 0x1FF);
    printf("ADC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_ADCVOLR, 0x1FF);
    printf("ADC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    /* Input path: LINPUT1/RINPUT1 -> PGA, MIC to boost */
    ret = WM8960_WriteReg(dev, WM8960_REG_LINPATH, 0x108);
    printf("MIC Path: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_RINPATH, 0x108);
    printf("MIC Path: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* PGA volume: 0dB, IPVU=1 to latch both channels */
    ret = WM8960_WriteReg(dev, WM8960_REG_LINVOL, 0x13F);
    printf("MIC Vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_RINVOL, 0x13F);
    printf("MIC Vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* Output mixer: DAC -> output (LD2LO=1, RD2RO=1) */
    ret = WM8960_WriteReg(dev, WM8960_REG_LOUTMIX, 0x100);
    printf("LMIX: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_ROUTMIX, 0x100);
    printf("RMIX: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* Headphone volume: 0dB with update bit */
    ret = WM8960_WriteReg(dev, WM8960_REG_LOUT1, 0x179);
    printf("LOUT1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_ROUT1, 0x179);
    printf("LOUT1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */

    /* Speaker: power up, 0dB volume, Class D enable */
    ret = WM8960_WriteReg(dev, WM8960_REG_POWER2, 0x1F8);
    printf("Speaker power: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_LSPKVOL, 0x179);
    printf("Speaker Vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_RSPKVOL, 0x179);
    printf("Speaker Vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    if (ret != HAL_OK) return ret;
    HAL_Delay(10); /* VMID charge-up */
    ret = WM8960_WriteReg(dev, WM8960_REG_CLASSD1, 0x0C0);
    //printf("Enable Class D : %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    //if (ret != HAL_OK) return ret;
    //HAL_Delay(10); /* VMID charge-up */
    
    printf("WM8960 Init OK\r\n");
    return HAL_OK;
}

/* ============================================================================
 * DAC
 * ============================================================================ */
HAL_StatusTypeDef WM8960_DAC_Enable(WM8960_Handle_t *dev)
{
    /* DACL=1, DACR=1 in Power2 — keep existing bits */
    return WM8960_WriteReg(dev, WM8960_REG_POWER2, 0x1F8);
}

HAL_StatusTypeDef WM8960_DAC_SetVolume(WM8960_Handle_t *dev, uint8_t volume)
{
    /* volume: 0=mute, 255=0dB. DACVU bit (bit 8) latches both channels */
    uint16_t val = ((uint16_t)volume) | 0x100;  // set DACVU
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_DACVOL1, val);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_DACVOL2, val);
}

HAL_StatusTypeDef WM8960_DAC_Mute(WM8960_Handle_t *dev)
{
    dev->dac_muted = 1;
    /* DACMU bit [3] in reg 0x05 */
    return WM8960_WriteReg(dev, WM8960_REG_ADCDAC, 0x008);
}

HAL_StatusTypeDef WM8960_DAC_Unmute(WM8960_Handle_t *dev)
{
    dev->dac_muted = 0;
    return WM8960_WriteReg(dev, WM8960_REG_ADCDAC, 0x000);
}

/* ============================================================================
 * ADC
 * ============================================================================ */
HAL_StatusTypeDef WM8960_ADC_Enable(WM8960_Handle_t *dev)
{
    /* ADCL=1, ADCR=1 already in Power1 from Init — this re-confirms */
    return WM8960_WriteReg(dev, WM8960_REG_POWER1, 0x0FE);
}

HAL_StatusTypeDef WM8960_ADC_SetVolume(WM8960_Handle_t *dev, uint8_t volume)
{
    /* volume: 0=mute, 255=max. ADCVU latches both */
    uint16_t val = ((uint16_t)volume) | 0x100;
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_ADCVOLL, val);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_ADCVOLR, val);
}

HAL_StatusTypeDef WM8960_ADC_Mute(WM8960_Handle_t *dev)
{
    dev->adc_muted = 1;
    /* ADCMU bits [0] and [1] in reg 0x05 */
    return WM8960_WriteReg(dev, WM8960_REG_ADCDAC, 0x003);
}

HAL_StatusTypeDef WM8960_ADC_Unmute(WM8960_Handle_t *dev)
{
    dev->adc_muted = 0;
    return WM8960_WriteReg(dev, WM8960_REG_ADCDAC, 0x000);
}

/* ============================================================================
 * MICROPHONE
 * ============================================================================ */
HAL_StatusTypeDef WM8960_Mic_Enable(WM8960_Handle_t *dev, WM8960_Input_t input)
{
    dev->input = input;
    uint16_t lpath, rpath;

    switch (input)
    {
        case WM8960_INPUT_MIC1:
            lpath = 0x108;  // LMN1=1, LMIC2B=1
            rpath = 0x108;
            break;
        case WM8960_INPUT_MIC2:
            lpath = 0x148;  // LMN2=1, LMIC2B=1
            rpath = 0x148;
            break;
        case WM8960_INPUT_MIC3:
            lpath = 0x178;  // LMN3=1, LMIC2B=1
            rpath = 0x178;
            break;
        case WM8960_INPUT_LINE:
            lpath = 0x100;  // bypass boost
            rpath = 0x100;
            break;
        default:
            lpath = 0x108;
            rpath = 0x108;
            break;
    }

    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LINPATH, lpath);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_RINPATH, rpath);
}

HAL_StatusTypeDef WM8960_Mic_SetBoost(WM8960_Handle_t *dev, WM8960_MicBoost_t boost)
{
    /* LMICBOOST[5:4] and RMICBOOST[5:4] in input path registers */
    uint16_t lpath = 0x108 | (uint16_t)boost;
    uint16_t rpath = 0x108 | (uint16_t)boost;
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LINPATH, lpath);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_RINPATH, rpath);
}

HAL_StatusTypeDef WM8960_Mic_SetPGAVolume(WM8960_Handle_t *dev, uint8_t volume)
{
    /* volume: 0-63. IPVU (bit 8) latches both channels simultaneously */
    uint16_t val = ((uint16_t)(volume & 0x3F)) | 0x100;
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LINVOL, val);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_RINVOL, val);
}

/* ============================================================================
 * HEADPHONE OUTPUT
 * ============================================================================ */
HAL_StatusTypeDef WM8960_Headphone_Enable(WM8960_Handle_t *dev)
{
    /* LOUT1=1, ROUT1=1 — keep DAC and speaker bits */
    return WM8960_WriteReg(dev, WM8960_REG_POWER2, 0x1F8);
}

HAL_StatusTypeDef WM8960_Headphone_SetVolume(WM8960_Handle_t *dev, uint8_t volume)
{
    /* volume: 0x30 (-73dB) to 0x7F (+6dB). 0x79 = 0dB
     * OUT1VU (bit 8) latches both channels */
    uint16_t val = ((uint16_t)(volume & 0x7F)) | 0x100;
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LOUT1, val);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_ROUT1, val);
}

HAL_StatusTypeDef WM8960_Headphone_Mute(WM8960_Handle_t *dev)
{
    /* Set volume to minimum (below 0x30 = mute) */
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LOUT1, 0x100);  // OUT1VU=1, vol=0
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_ROUT1, 0x100);
}

HAL_StatusTypeDef WM8960_Headphone_Unmute(WM8960_Handle_t *dev)
{
    return WM8960_Headphone_SetVolume(dev, WM8960_HP_VOL_0DB);
}

/* ============================================================================
 * SPEAKER OUTPUT (Class D)
 * ============================================================================ */
HAL_StatusTypeDef WM8960_Speaker_Enable(WM8960_Handle_t *dev)
{
    HAL_StatusTypeDef ret;
    /* SPKL=1, SPKR=1 in Power2 */
    ret = WM8960_WriteReg(dev, WM8960_REG_POWER2, 0x1F8);
    if (ret != HAL_OK) return ret;
    /* Class D enable: SPK_OP_EN[1:0] = 11 */
    return WM8960_WriteReg(dev, WM8960_REG_CLASSD1, 0x0C0);
}

HAL_StatusTypeDef WM8960_Speaker_SetVolume(WM8960_Handle_t *dev, uint8_t volume)
{
    /* volume: 0x30 (-73dB) to 0x7F (+6dB). SPKVU (bit 8) latches both */
    uint16_t val = ((uint16_t)(volume & 0x7F)) | 0x100;
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LSPKVOL, val);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_RSPKVOL, val);
}

HAL_StatusTypeDef WM8960_Speaker_Mute(WM8960_Handle_t *dev)
{
    HAL_StatusTypeDef ret;
    ret = WM8960_WriteReg(dev, WM8960_REG_LSPKVOL, 0x100);
    if (ret != HAL_OK) return ret;
    return WM8960_WriteReg(dev, WM8960_REG_RSPKVOL, 0x100);
}

HAL_StatusTypeDef WM8960_Speaker_Unmute(WM8960_Handle_t *dev)
{
    return WM8960_Speaker_SetVolume(dev, WM8960_HP_VOL_0DB);
}


