/*
 * Copyright (c) 2020 Ruslan V. Uss <unclerus@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of itscontributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file ina226.h
 * @defgroup ina226 ina226
 * @{
 *
 * adapted from ina226  by Dig Kleppe <digKleppe@gmail.com>
 *
 * ESP-IDF driver for INA226 precision digital current and power monitor
 *
 * Copyright (c) 2020 Ruslan V. Uss <unclerus@gmail.com>
 *
 *
 */


#ifndef __INA226_H__
#define __INA226_H__

#include <i2cdev.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INA226_ADDR_PIN_GND 0x00
#define INA226_ADDR_PIN_VS  0x01
#define INA226_ADDR_PIN_SDA 0x02
#define INA226_ADDR_PIN_SCL 0x03

/**
 * Macro to define I2C address
 *
 * Examples:
 *    INA226_ADDR(INA226_ADDR_PIN_GND, INA226_ADDR_PIN_GND) = 0x40 (A0 = A1 = GND)
 *    INA226_ADDR(INA226_ADDR_PIN_VS, INA226_ADDR_PIN_SDA)  = 0x49 (A0 = VS, A1 = SDA)
 */
#define INA226_ADDR(A0, A1) (0x40 | ((A1) << 2) | (A0))

/**
 * Averaging mode.
 * Determines the number of samples that are collected and averaged.
 */
typedef enum
{
    INA226_AVG_1 = 0, //!< 1 sample, default
    INA226_AVG_4,     //!< 4 samples
    INA226_AVG_16,    //!< 16 samples
    INA226_AVG_64,    //!< 64 samples
    INA226_AVG_128,   //!< 128 samples
    INA226_AVG_256,   //!< 256 samples
    INA226_AVG_512,   //!< 512 samples
    INA226_AVG_1024   //!< 1024 samples
} ina226_averaging_mode_t;

/**
 * Conversion time
 */
typedef enum
{
    INA226_CT_140 = 0, //!< 140 us
    INA226_CT_204,     //!< 204 us
    INA226_CT_332,     //!< 332 us
    INA226_CT_588,     //!< 588 us
    INA226_CT_1100,    //!< 1.1 ms, default
    INA226_CT_2116,    //!< 2.116 ms
    INA226_CT_4156,    //!< 4.156 ms
    INA226_CT_8244,    //!< 8.244 ms
} ina226_conversion_time_t;

/**
 * Operating mode
 */
typedef enum
{
    INA226_MODE_POWER_DOWN     = 0, //!< Power-done
    INA226_MODE_TRIG_SHUNT     = 1, //!< Shunt current, triggered
    INA226_MODE_TRIG_BUS       = 2, //!< Bus voltage, triggered
    INA226_MODE_TRIG_SHUNT_BUS = 3, //!< Shunt current and bus voltage, triggered
    INA226_MODE_POWER_DOWN2    = 4, //!< Power-done
    INA226_MODE_CONT_SHUNT     = 5, //!< Shunt current, continuous
    INA226_MODE_CONT_BUS       = 6, //!< Bus voltage, continuous
    INA226_MODE_CONT_SHUNT_BUS = 7  //!< Shunt current and bus voltage, continuous (default)
} ina226_mode_t;

/**
 * Alert function mode
 */
typedef enum
{
    INA226_ALERT_DISABLED, //!< No alert function
    INA226_ALERT_OCL,      //!< Over current limit
    INA226_ALERT_UCL,      //!< Under current limit
    INA226_ALERT_BOL,      //!< Bus voltage over-voltage
    INA226_ALERT_BUL,      //!< Bus voltage under-voltage
    INA226_ALERT_POL,      //!< Power over-limit
} ina226_alert_mode_t;

/**
 * Device descriptor
 */
typedef struct
{
    i2c_dev_t i2c_dev; //!< I2C device descriptor
    uint16_t config;   //!< Current config
    uint16_t mfr_id;   //!< Manufacturer ID
    uint16_t die_id;   //!< Die ID
} ina226_t;

/**
 * @brief Initialize device descriptor.
 *
 * @param dev Device descriptor
 * @param addr Device I2C address
 * @param port I2C port
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return `ESP_OK` on success
 */
//esp_err_t ina226_init_desc(ina226_t *dev, uint8_t addr, i2c_port_t port,
//                           gpio_num_t sda_gpio, gpio_num_t scl_gpio);

esp_err_t ina226_init_desc(ina226_t *dev, uint8_t addr, i2c_port_t port ,  int clkSpeedHz);


/**
 * @brief Free device descriptor.
 *
 * @param dev Device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ina226_free_desc(ina226_t *dev);

/**
 * @brief Initialize device.
 *
 * Reads sensor configuration.
 *
 * @param dev Device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ina226_init(ina226_t *dev);

/**
 * @brief Reset sensor.
 *
 * @param dev Device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ina226_reset(ina226_t *dev);

/**
 * @brief Configure sensor.
 *
 * @param dev Device descriptor
 * @param mode Operating mode
 * @param avg_mode Averaging mode
 * @param vbus_ct Bus voltage conversion time
 * @param ish_ct Shunt current conversion time
 * @return `ESP_OK` on success
 */
esp_err_t ina226_set_config(ina226_t *dev, ina226_mode_t mode, ina226_averaging_mode_t avg_mode,
                            ina226_conversion_time_t vbus_ct, ina226_conversion_time_t ish_ct);

/**
 * @brief Read sensor configuration.
 *
 * @param dev Device descriptor
 * @param[out] mode Operating mode
 * @param[out] avg_mode Averaging mode
 * @param[out] vbus_ct Bus voltage conversion time
 * @param[out] ish_ct Shunt current conversion time
 * @return `ESP_OK` on success
 */
esp_err_t ina226_get_config(ina226_t *dev, ina226_mode_t *mode, ina226_averaging_mode_t *avg_mode,
                            ina226_conversion_time_t *vbus_ct, ina226_conversion_time_t *ish_ct);

/**
 * @brief Setup ALERT pin.
 *
 * @param dev Device descriptor
 * @param mode Alert function mode
 * @param limit Alert limit value
 * @param cvrf If true also assert ALERT pin when device is ready to next conversion
 * @param active_high Set active ALERT pin level is high
 * @param latch Enable latch mode on ALERT pin (see ::ina226_get_status())
 * @return `ESP_OK` on success
 */
esp_err_t ina226_set_alert(ina226_t *dev, ina226_alert_mode_t mode, float limit,
                           bool cvrf, bool active_high, bool latch);

/**
 * @brief Trigger single conversion.
 *
 * Function will return an error if current operating
 * mode is not `INA226_MODE_TRIG_SHUNT`/`INA226_MODE_TRIG_BUS`/`INA226_MODE_TRIG_SHUNT_BUS`
 *
 * @param dev Device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ina226_trigger(ina226_t *dev);

/**
 * @brief Get device status.
 *
 * This function also clears ALERT state if latch mode is enabled for ALERT pin.
 *
 * @param dev Device descriptor
 * @param[out] ready If true, device is ready for the next conversion
 * @param[out] alert If true, there was alert
 * @param[out] overflow If true, power data have exceeded max 419.43 W
 * @return `ESP_OK` on success
 */
esp_err_t ina226_get_status(ina226_t *dev, bool *ready, bool *alert, bool *overflow);

/**
 * @brief Read shunt voltage.
 *
 *
 * @param dev Device descriptor
 * @param[out] voltage V
 * @return `ESP_OK` on success
 */
esp_err_t ina226_get_shunt_voltage(ina226_t *dev, float *voltage);

/**
 * @brief Read bus voltage.
 *
 * @param dev Device descriptor
 * @param[out] voltage Bus voltage, V
 * @return `ESP_OK` on success
 */
esp_err_t ina226_get_bus_voltage(ina226_t *dev, float *voltage);


/**
 * @brief set calibrationregister
 *
 *
 * @param dev Device descriptor
 * @param[in] value
 * @return `ESP_OK` on success
*/

esp_err_t ina226_set_calibrationregister(ina226_t *dev, uint16_t value);

/**
 * @brief Read power.
 *
 * This function works properly only after setting the calibration register
 *
 * @param dev Device descriptor
 * @param[out] power Power, W
 * @return `ESP_OK` on success
 */
esp_err_t ina226_get_power(ina226_t *dev, float *power);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __INA226_H__ */
