#include "blind_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace blind_controller {

static const char *TAG = "blind_controller";

void BlindControllerComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Blind Controller...");

  if (this->auto_assign_address_) {
    uint8_t data;
    // Try to read register 3 to see if device exists at configured address
    if (this->read_bytes(3, &data, 1) != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "Device not found at configured address 0x%02X. Attempting to assign from default address 0x09...", this->address_);
      
      uint8_t target_addr = this->address_;
      this->set_i2c_address(0x09);
      
      // Check if ANY device is at 0x09
      if (this->read_bytes(3, &data, 1) == i2c::ERROR_OK) {
        ESP_LOGI(TAG, "Device found at default address 0x09. Changing its address to 0x%02X...", target_addr);
        uint8_t payload[4] = {0, 0, 0, target_addr};
        this->write_bytes(0, payload, 4);
        ESP_LOGI(TAG, "Address change sent. The device will reboot.");
      } else {
        ESP_LOGE(TAG, "No device found at default address 0x09 either.");
      }
      
      // Restore original address
      this->set_i2c_address(target_addr);
    }
  }
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

void BlindControllerComponent::set_auto_assign_address(bool auto_assign) {
  this->auto_assign_address_ = auto_assign;
}

}  // namespace blind_controller
}  // namespace esphome
