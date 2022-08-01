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
    union WhirlpoolProtocol{
    uint8_t raw[WHIRLPOOL_STATE_LENGTH];  ///< The state in IR code form
    struct {
      // Byte 0~1
      uint8_t pad0[2];
      // Byte 2
      uint8_t Fan     :2;
      uint8_t Power   :1;
      uint8_t Sleep   :1;
      uint8_t         :3;
      uint8_t Swing1  :1;
      // Byte 3
      uint8_t Mode  :3;
      uint8_t       :1;
      uint8_t Temp  :4;
      // Byte 4
      uint8_t Auto6  :8;
      // Byte 5
      uint8_t         :4;
      uint8_t Super1  :1;
      uint8_t         :2;
      uint8_t Super2  :1;
      // Byte 6
      uint8_t ClockHours  :5;
      uint8_t LightOff    :1;
      uint8_t             :2;
      // Byte 7
      uint8_t ClockMins       :6;
      uint8_t                 :1;
      uint8_t OffTimerEnabled :1;
      // Byte 8
      uint8_t OffHours  :5;
      uint8_t           :1;
      uint8_t Swing2    :1;
      uint8_t           :1;
      // Byte 9
      uint8_t OffMins         :6;
      uint8_t                 :1;
      uint8_t OnTimerEnabled  :1;
      // Byte 10
      uint8_t OnHours :5;
      uint8_t         :3;
      // Byte 11
      uint8_t OnMins  :6;
      uint8_t         :2;
      // Byte 12
      uint8_t       :8;
      // Byte 13
      uint8_t Sum1  :8;
      // Byte 14
      uint8_t       :8;
      // Byte 15
      uint8_t Cmd   :8;
      // Byte 16~17
      uint8_t pad1[2];
      // Byte 18
      uint8_t       :3;
      uint8_t J191  :1;
      uint8_t       :4;
      // Byte 19
      uint8_t       :8;
      // Byte 20
      uint8_t Sum2  :8;
    };
  };

  WhirlpoolProtocol remote_state;

  // uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  for (uint8_t i = 2; i < WHIRLPOOL_STATE_LENGTH; i++) remote_state.raw[i] = 0x0;
  remote_state.raw[0] = 0x83;
  remote_state.raw[1] = 0x06;
  remote_state.raw[6] = 0x80;
  if(model_ == MODEL_DG11J1_91) {
    // MODEL DG11J191
    remote_state.raw[18] = 0x08;
  }

  auto powered_on = this->mode != climate::CLIMATE_MODE_OFF;
  if (powered_on != this->powered_on_assumed) {
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
  remote_state.Temp = (uint8_t)(temp - this->temperature_min_());

  // Fan speed
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      remote_state.Fan = WHIRLPOOL_FAN_HIGH;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      remote_state.Fan = WHIRLPOOL_FAN_MED;
      break;
    case climate::CLIMATE_FAN_LOW:
      remote_state.Fan = WHIRLPOOL_FAN_LOW;
      break;
    default:
      break;
  }

  // Swing
  ESP_LOGV(TAG, "send swing %s", this->send_swing_cmd_ ? "true" : "false");
  if (this->send_swing_cmd_) {
    if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL) {
      remote_state.Swing1 = true;
      remote_state.Swing2 = true;
    } else if(this->swing_mode == climate::CLIMATE_SWING_OFF){
      remote_state.Swing1 = false;
      remote_state.Swing2 = false;
    }
  }

  // Checksum
  for (uint8_t i = 2; i < 13; i++)
    remote_state.raw[13] ^= remote_state.raw[i];
  for (uint8_t i = 14; i < 20; i++)
    remote_state.raw[20] ^= remote_state.raw[i];

  ESP_LOGV(TAG,
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

  transmit.perform();
}

bool WhirlpoolClimate::on_receive(remote_base::RemoteReceiveData data) {
  // Validate header
  if (!data.expect_item(WHIRLPOOL_HEADER_MARK, WHIRLPOOL_HEADER_SPACE)) {
    ESP_LOGV(TAG, "Header fail");
    return false;
  }

  uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  // Read all bytes.
  for (int i = 0; i < WHIRLPOOL_STATE_LENGTH; i++) {
    // Read bit
    if (i == 6 || i == 14) {
      if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_GAP))
        return false;
    }
    for (int j = 0; j < 8; j++) {
      if (data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ONE_SPACE)) {
        remote_state[i] |= 1 << j;

      } else if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ZERO_SPACE)) {
        ESP_LOGV(TAG, "Byte %d bit %d fail", i, j);
        return false;
      }
    }

    ESP_LOGVV(TAG, "Byte %d %02X", i, remote_state[i]);
  }
  // Validate footer
  if (!data.expect_mark(WHIRLPOOL_BIT_MARK)) {
    ESP_LOGV(TAG, "Footer fail");
    return false;
  }

  uint8_t checksum13 = 0;
  uint8_t checksum20 = 0;
  // Calculate  checksum and compare with signal value.
  for (uint8_t i = 2; i < 13; i++)
    checksum13 ^= remote_state[i];
  for (uint8_t i = 14; i < 20; i++)
    checksum20 ^= remote_state[i];

  if (checksum13 != remote_state[13] || checksum20 != remote_state[20]) {
    ESP_LOGVV(TAG, "Checksum fail");
    return false;
  }

  ESP_LOGV(
      TAG,
      "Received: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
      "%02X %02X   %02X",
      remote_state[0], remote_state[1], remote_state[2], remote_state[3], remote_state[4], remote_state[5],
      remote_state[6], remote_state[7], remote_state[8], remote_state[9], remote_state[10], remote_state[11],
      remote_state[12], remote_state[13], remote_state[14], remote_state[15], remote_state[16], remote_state[17],
      remote_state[18], remote_state[19], remote_state[20]);

  // verify header remote code
  if (remote_state[0] != 0x83 || remote_state[1] != 0x06)
    return false;

  // powr on/off button
  ESP_LOGV(TAG, "Power: %02X", (remote_state[2] & WHIRLPOOL_POWER));

  if ((remote_state[2] & WHIRLPOOL_POWER) == WHIRLPOOL_POWER) {
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
    auto mode = remote_state[3] & 0x7;
    ESP_LOGV(TAG, "Mode: %02X", mode);
    switch (mode) {
      case WHIRLPOOL_HEAT:
        if((remote_state[2] & WHIRLPOOL_POWER) == WHIRLPOOL_POWER) {
          this->mode = climate::CLIMATE_MODE_OFF;
        } else {
          this->mode = climate::CLIMATE_MODE_HEAT;
        }
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
  int temp = remote_state[3] & 0xF0;
  ESP_LOGVV(TAG, "Temperature Raw: %02X", temp);
  temp = (uint8_t) temp >> 4;
  temp += static_cast<int>(this->temperature_min_());
  ESP_LOGVV(TAG, "Temperature Climate: %u", temp);
  this->target_temperature = temp;

  // Set received fan speed
  auto fan = remote_state[2] & 0x03;
  ESP_LOGVV(TAG, "Fan: %02X", fan);
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
  if ((remote_state[2] & WHIRLPOOL_SWING_MASK) == WHIRLPOOL_SWING_MASK && remote_state[8] == 0x40) {
    ESP_LOGVV(TAG, "Swing toggle pressed ");
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
