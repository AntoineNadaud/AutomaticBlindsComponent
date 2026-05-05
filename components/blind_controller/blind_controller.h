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

 protected:
  void control(const cover::CoverCall &call) override;

 private:
  uint8_t speed_{30};
};

}  // namespace blind_controller
}  // namespace esphome
