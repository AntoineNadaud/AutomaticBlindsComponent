#include "blind_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace blind_controller {

static const char *TAG = "blind_controller";

void BlindControllerComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Blind Controller...");
}

void BlindControllerComponent::loop() {
  uint8_t data[3];
  if (this->read_bytes(0, data, 3) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  // data[0] is command/status. Bit 0 = homing.
  // data[1] is target position
  // data[2] is current position

  float current_pos = 1.0f - (data[2] / 255.0f);
  if (std::abs(this->position - current_pos) > 0.02f) {
    this->position = current_pos;
    this->publish_state();
  }
}

void BlindControllerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Blind Controller:");
  LOG_I2C_DEVICE(this);
}

cover::CoverTraits BlindControllerComponent::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  return traits;
}

void BlindControllerComponent::control(const cover::CoverCall &call) {
  uint8_t cmd[2] = {0, 0};
  cmd[0] = (this->speed_ & 0x3F) << 2; // Speed in bits 2-7
  cmd[0] |= 0x02; // Sensor on (bit 1)

  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    uint8_t target = (uint8_t)((1.0f - pos) * 255.0f);
    cmd[1] = target;

    // Send register address 0, then the two bytes
    this->write_bytes(0, cmd, 2);
  }
}

void BlindControllerComponent::set_speed(uint8_t speed) {
  this->speed_ = speed;
}

}  // namespace blind_controller
}  // namespace esphome
