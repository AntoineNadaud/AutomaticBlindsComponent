#include "blind_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace blind_controller {

static const char *TAG = "blind_controller";

void BlindControllerComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Blind Controller...");
  
  // Schedule the initialization of the sensor
  this->set_timeout("init_sensor", this->sensor_init_delay_, [this]() {
    this->initialize_homing_sensor();
  });
}

void BlindControllerComponent::initialize_homing_sensor() {
  ESP_LOGI(TAG, "Initializing VL53L0X Homing Sensor...");

  // 1. Turn on the sensor via the CH32V003 (set bit 1 of register 0)
  uint8_t cmd[1];
  if (this->read_bytes(0, cmd, 1) == i2c::ERROR_OK) {
    cmd[0] |= 0x02; // Bit 1 = Sensor ON
    this->write_bytes(0, cmd, 1);
  } else {
    ESP_LOGE(TAG, "Failed to communicate with blind controller to enable sensor.");
    return;
  }

  // Allow time for the VL53L0X to power up
  delay(15); 
  
  uint8_t original_addr = this->address_;

  // 2. Change the VL53L0X I2C address
  // The VL53L0X defaults to 0x29 upon power-up
  this->set_i2c_address(0x29);
  
  // 0x8A is the I2C_SLAVE_DEVICE_ADDRESS register on VL53L0X
  if (this->sensor_address_ != 0x29) {
    this->write_byte(0x8A, this->sensor_address_ & 0x7F);
  }

  // 3. Target the new sensor address for configuration
  this->set_i2c_address(this->sensor_address_);

  // Configure GPIO Interrupt Threshold
  // System Interrupt Config GPIO (0x0A) -> 0x01 (Level Low, trigger when < threshold)
  this->write_byte(0x0A, 0x01);

  // Set Low Threshold in mm (0x32, 0x33)
  this->write_byte(0x32, (this->trigger_distance_ >> 8) & 0xFF);
  this->write_byte(0x33, this->trigger_distance_ & 0xFF);

  // Clear any existing interrupt mask (0x0B)
  this->write_byte(0x0B, 0x01);

  // Start continuous ranging (SYSRANGE_START 0x00 -> 0x02)
  this->write_byte(0x00, 0x02);

  // Restore the component's I2C address back to the CH32V003 motor controller
  this->set_i2c_address(original_addr);

  ESP_LOGI(TAG, "VL53L0X initialized on address 0x%02X with %umm threshold.", this->sensor_address_, this->trigger_distance_);
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

void BlindControllerComponent::set_speed(uint8_t speed) { this->speed_ = speed; }
void BlindControllerComponent::set_sensor_address(uint8_t address) { this->sensor_address_ = address; }
void BlindControllerComponent::set_trigger_distance(uint16_t distance) { this->trigger_distance_ = distance; }
void BlindControllerComponent::set_sensor_init_delay(uint32_t delay) { this->sensor_init_delay_ = delay; }

}  // namespace blind_controller
}  // namespace esphome
