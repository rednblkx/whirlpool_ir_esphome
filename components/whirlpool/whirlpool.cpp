#include "whirlpool.h"
#include "esphome/core/log.h"

namespace esphome {
namespace whirlpool {

static const char *const TAG = "whirlpool.climate";

const uint16_t WHIRLPOOL_HEADER_MARK = 9000;
const uint16_t WHIRLPOOL_HEADER_SPACE = 4494;
const uint16_t WHIRLPOOL_BIT_MARK = 572;
const uint16_t WHIRLPOOL_ONE_SPACE = 1659;
const uint16_t WHIRLPOOL_ZERO_SPACE = 553;
const uint32_t WHIRLPOOL_GAP = 7960;

const uint32_t WHIRLPOOL_CARRIER_FREQUENCY = 38000;

const uint8_t WHIRLPOOL_STATE_LENGTH = 21;

const uint8_t WHIRLPOOL_HEAT = 0;
const uint8_t WHIRLPOOL_DRY = 3;
const uint8_t WHIRLPOOL_COOL = 2;
const uint8_t WHIRLPOOL_FAN = 4;
const uint8_t WHIRLPOOL_AUTO = 1;

const uint8_t WHIRLPOOL_FAN_AUTO = 0;
const uint8_t WHIRLPOOL_FAN_HIGH = 1;
const uint8_t WHIRLPOOL_FAN_MED = 2;
const uint8_t WHIRLPOOL_FAN_LOW = 3;

const uint8_t WHIRLPOOL_SWING_MASK = 128;

const uint8_t WHIRLPOOL_POWER = 0x04;

void WhirlpoolClimate::transmit_state() {

  WhirlpoolProtocol remote_state;

  // uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  remote_state.raw[0] = 0x83;
  remote_state.raw[1] = 0x06;
  remote_state.raw[6] = 0x80;
  if(model_ == MODEL_DG11J1_91) {
    // MODEL DG11J191
    remote_state.raw[18] = 0x08;
  }

  auto powered_on = this->mode != climate::CLIMATE_MODE_OFF;
  if (powered_on != this->powered_on_assumed) {
    remote_state.Swing1 = false;
    remote_state.Swing2 = false;
    // Set power toggle command
    remote_state.Power = true;
    remote_state.Cmd = 0x01;
    this->powered_on_assumed = powered_on;
  }
  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT_COOL:
      // set fan auto
      // set temp auto temp
      // set sleep false
      remote_state.Mode = WHIRLPOOL_AUTO;
      remote_state.Temp = (uint8_t)(25 - this->temperature_min_());
      remote_state.Auto6 = (uint8_t)0x80;
      remote_state.Cmd = (uint8_t)0x17;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state.Mode = WHIRLPOOL_HEAT;
      remote_state.Cmd = 6;
      break;
    case climate::CLIMATE_MODE_COOL:
      remote_state.Mode = WHIRLPOOL_COOL;
      remote_state.Cmd = 6;
      break;
    case climate::CLIMATE_MODE_DRY:
      remote_state.Mode = WHIRLPOOL_DRY;
      remote_state.Cmd = 6;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      remote_state.Mode = WHIRLPOOL_FAN;
      remote_state.Cmd = 6;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      break;
  }
  // Temperature
  auto temp = (uint8_t) roundf(clamp(this->target_temperature, this->temperature_min_(), this->temperature_max_()));
  if(this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
    temp = (uint8_t) roundf(clamp(this->target_temperature, this->temperature_min_() + 1, this->temperature_max_()));
    if(temp - 25 > 0) {
      remote_state.AutoTempOffset = clamp(temp - 26, 0, 2);
    } else if(temp - 26 < 0) {
      remote_state.AutoTempOffset = clamp((26 - temp) + 4, 5, 6);
    }
    if(temp - 26 >= 3){
      remote_state.pad1[0] = (uint8_t)(((temp - 25) - 3) + 1) * 4;
    }
    if(temp - 26 <= -3){
      remote_state.pad1[0] = (uint8_t)(((26 - temp) - 3) + 1) * 4;
    }
    remote_state.Temp = (uint8_t)(25 - this->temperature_min_());
  } else {
    remote_state.Temp = (uint8_t)(temp - this->temperature_min_());
  }

  if(temp_change){
    remote_state.Cmd = 0x02;
  }

  // if(mode_change && this->mode != climate::CLIMATE_MODE_HEAT_COOL && this->mode != climate::CLIMATE_MODE_OFF){
  //   remote_state.Cmd = 0x06;
  // }
  // Fan speed
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      remote_state.Fan = WHIRLPOOL_FAN_HIGH;
      remote_state.Cmd = 0x11;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      remote_state.Fan = WHIRLPOOL_FAN_MED;
      remote_state.Cmd = 0x11;
      break;
    case climate::CLIMATE_FAN_LOW:
      remote_state.Fan = WHIRLPOOL_FAN_LOW;
      remote_state.Cmd = 0x11;
      break;
    default:
      break;
  }

  // Swing
  ESP_LOGD(TAG, "send swing %s", this->send_swing_cmd_ ? "true" : "false");
  if (this->send_swing_cmd_) {
      remote_state.Swing1 = true;
      remote_state.Swing2 = true;
      remote_state.Cmd = 0x07;
  }

  if(remote_state.Cmd != 0x17) {
    remote_state.Auto6 = 0;
  }

  // Checksum
  for (uint8_t i = 2; i < 13; i++)
    remote_state.raw[13] ^= remote_state.raw[i];
  for (uint8_t i = 14; i < 20; i++)
    remote_state.raw[20] ^= remote_state.raw[i];

  ESP_LOGD(TAG,
           "Sending: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
           "%02X %02X   %02X",
           remote_state.raw[0], remote_state.raw[1], remote_state.raw[2], remote_state.raw[3], remote_state.raw[4], remote_state.raw[5],
           remote_state.raw[6], remote_state.raw[7], remote_state.raw[8], remote_state.raw[9], remote_state.raw[10], remote_state.raw[11],
           remote_state.raw[12], remote_state.raw[13], remote_state.raw[14], remote_state.raw[15], remote_state.raw[16], remote_state.raw[17],
           remote_state.raw[18], remote_state.raw[19], remote_state.raw[20]);

  // Send code
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(38000);

  // Header
  data->mark(WHIRLPOOL_HEADER_MARK);
  data->space(WHIRLPOOL_HEADER_SPACE);
  // Data
  auto bytes_sent = 0;
  for (uint8_t i : remote_state.raw) {
    for (uint8_t j = 0; j < 8; j++) {
      data->mark(WHIRLPOOL_BIT_MARK);
      bool bit = i & (1 << j);
      data->space(bit ? WHIRLPOOL_ONE_SPACE : WHIRLPOOL_ZERO_SPACE);
    }
    bytes_sent++;
    if (bytes_sent == 6 || bytes_sent == 14) {
      // Divider
      data->mark(WHIRLPOOL_BIT_MARK);
      data->space(WHIRLPOOL_GAP);
    }
  }
  // Footer
  data->mark(WHIRLPOOL_BIT_MARK);

  this->recent_transmit = true;

  if(this->transmit_enabled){
    transmit.perform();
  }


  set_timeout(1000, [this]() { this->recent_transmit = false; });
}

bool WhirlpoolClimate::on_receive(remote_base::RemoteReceiveData data) {
  if(recent_transmit) {
    this->recent_transmit = false;
    ESP_LOGD(TAG, "Received message, but ignoring because of recent transmit in order to avoid double state change");
    return false;
  }
  // Validate header
  if (!data.expect_item(WHIRLPOOL_HEADER_MARK, WHIRLPOOL_HEADER_SPACE)) {
    ESP_LOGD(TAG, "Header fail");
    return false;
  }

  WhirlpoolProtocol remote_state;

  // uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  // Read all bytes.
  for (int i = 0; i < WHIRLPOOL_STATE_LENGTH; i++) {
    // Read bit
    if (i == 6 || i == 14) {
      if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_GAP))
        return false;
    }
    for (int j = 0; j < 8; j++) {
      if (data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ONE_SPACE)) {
        remote_state.raw[i] |= 1 << j;

      } else if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ZERO_SPACE)) {
        ESP_LOGD(TAG, "Byte %d bit %d fail", i, j);
        return false;
      }
    }

    ESP_LOGD(TAG, "Byte %d %02X", i, remote_state.raw[i]);
  }
  // Validate footer
  if (!data.expect_mark(WHIRLPOOL_BIT_MARK)) {
    ESP_LOGD(TAG, "Footer fail");
    return false;
  }

  uint8_t checksum13 = 0;
  uint8_t checksum20 = 0;
  // Calculate  checksum and compare with signal value.
  for (uint8_t i = 2; i < 13; i++)
    checksum13 ^= remote_state.raw[i];
  for (uint8_t i = 14; i < 20; i++)
    checksum20 ^= remote_state.raw[i];

  if (checksum13 != remote_state.raw[13] || checksum20 != remote_state.raw[20]) {
    ESP_LOGD(TAG, "Checksum fail");
    return false;
  }

  ESP_LOGD(
      TAG,
      "Received: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
      "%02X %02X   %02X",
      remote_state.raw[0], remote_state.raw[1], remote_state.raw[2], remote_state.raw[3], remote_state.raw[4], remote_state.raw[5],
      remote_state.raw[6], remote_state.raw[7], remote_state.raw[8], remote_state.raw[9], remote_state.raw[10], remote_state.raw[11],
      remote_state.raw[12], remote_state.raw[13], remote_state.raw[14], remote_state.raw[15], remote_state.raw[16], remote_state.raw[17],
      remote_state.raw[18], remote_state.raw[19], remote_state.raw[20]);

  // verify header remote code
  if (remote_state.raw[0] != 0x83 || remote_state.raw[1] != 0x06)
    return false;

  // powr on/off button
  ESP_LOGD(TAG, "Power: %02X", (remote_state.raw[2] & WHIRLPOOL_POWER));

  if (remote_state.Power) {
    auto powered_on = this->mode != climate::CLIMATE_MODE_OFF;

    if (powered_on) {
      this->mode = climate::CLIMATE_MODE_OFF;
      this->powered_on_assumed = false;
    } else {
      this->powered_on_assumed = true;
    }
  }

  // Set received mode
  if (powered_on_assumed) {
    auto mode = remote_state.raw[3] & 0x7;
    ESP_LOGD(TAG, "Mode: %02X", mode);
    switch (mode) {
      case WHIRLPOOL_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case WHIRLPOOL_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case WHIRLPOOL_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case WHIRLPOOL_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case WHIRLPOOL_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
    }
  }

  // Set received temp
  int temp = remote_state.Temp;
  ESP_LOGD(TAG, "Temperature Raw: %02X", temp);
  // temp = (uint8_t) temp >> 4;
  temp += static_cast<int>(this->temperature_min_());
  ESP_LOGD(TAG, "Temperature Climate: %u", temp);
  if(remote_state.Mode == WHIRLPOOL_AUTO) {
    if(remote_state.AutoTempOffset > 0 && remote_state.AutoTempOffset <= 2) {
      this->target_temperature = temp + (remote_state.AutoTempOffset + (remote_state.pad1[0] / 4));
    } else if(remote_state.AutoTempOffset >= 5) {
      this->target_temperature = temp - ((remote_state.AutoTempOffset % 4) + (remote_state.pad1[0] / 4)) + 1;
    } else this->target_temperature = temp + 1;
  }else {
    this->target_temperature = temp;
  }

  // Set received fan speed
  auto fan = remote_state.raw[2] & 0x03;
  ESP_LOGD(TAG, "Fan: %02X", fan);
  switch (fan) {
    case WHIRLPOOL_FAN_HIGH:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case WHIRLPOOL_FAN_MED:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case WHIRLPOOL_FAN_LOW:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case WHIRLPOOL_FAN_AUTO:
    default:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }

  // Set received swing status
  if (remote_state.Swing1 && remote_state.Swing2 && remote_state.Cmd == 0x07) {
    ESP_LOGD(TAG, "Swing toggle pressed ");
    if (this->swing_mode == climate::CLIMATE_SWING_OFF) {
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    } else {
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    }
  }

  this->publish_state();
  return true;
}

}  // namespace whirlpool
}  // namespace esphome
