#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace blind_controller {

class BlindControllerComponent : public cover::Cover, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  cover::CoverTraits get_traits() override;
  void set_speed(uint8_t speed);
  
  void set_sensor_address(uint8_t address);
  void set_trigger_distance(uint16_t distance);
  void set_sensor_init_delay(uint32_t delay);

  // Method to configure the VL53L0X
  void initialize_homing_sensor();

 protected:
  void control(const cover::CoverCall &call) override;

 private:
  uint8_t speed_{30};
  uint8_t sensor_address_{0x29};
  uint16_t trigger_distance_{100};
  uint32_t sensor_init_delay_{5000};
};

}  // namespace blind_controller
}  // namespace esphome
