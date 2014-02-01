/*
Copyright (c) 2013, Silas Parker
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    The name of Silas Parker may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <sstream>

#include <scssdk_telemetry.h>
#include <eurotrucks2/scssdk_telemetry_eut2.h>

#include "serial.hpp"
#include "options.hpp"

static const float METERS_PER_SEC_TO_MILES_PER_HOUR = 2.2369f;
static const float METERS_PER_SEC_TO_KM_PER_HOUR = 3.6f;

// Game log file
scs_log_t game_log = NULL;

// COM port file
Serial serial_port;

unsigned long last_update = 0;

// Packet
#define PACKET_MAX_SIZE 720 // Can send a maximum of 720 bytes per frame at 20 FPS over a 115200 bps line
#define PACKET_SYNC 0xFF
#define PACKET_VER  2

unsigned char packet[PACKET_MAX_SIZE];


#define PACKBOOL(A,B,C,D,E,F,G,H) \
  (((unsigned char)(A) << 7) | ((unsigned char)(B) << 6) | \
  ((unsigned char)(C) << 5) | ((unsigned char)(D) << 4) | \
  ((unsigned char)(E) << 3) | ((unsigned char)(F) << 2) | \
  ((unsigned char)(G) << 1) | ((unsigned char)(H)))


// Telemetry data
struct telemetry_state_t
{
  float	speed; // Meters per Second
	float	engine_rpm; // RPM

  int   engine_gear; // >0 Forward, 0 Neutral, <0 Reverse
  
  bool  parking_brake;
  bool  motor_brake;
  
  float brake_air_pressure; // PSI
  bool  brake_air_pressure_warning;
  bool  brake_air_pressure_emergency;
  float brake_temperature;
  
  float fuel; // Liters
  bool  fuel_warning;
  float fuel_average_consumption; // Liters / KM
  float fuel_capacity; // Liters
    
  float oil_pressure; // PSI
  bool  oil_pressure_warning;
  float oil_temperature; // Celsius
  
  float water_temperature; // Celsius
  bool  water_temperature_warning;
  
  float battery_voltage; // Volts
  bool  battery_voltage_warning;
  
  bool  electric_enabled;
  bool  engine_enabled;
  
  bool  light_lblinker;
  bool  light_rblinker;
  bool  light_parking;
  bool  light_low_beam;  // Dipped headlights
  bool  light_high_beam; // Full headlights
  bool  light_brake;
  bool  light_reverse;

  float odometer; // Kilometres
  
} telemetry;


Options option_file;


unsigned char float_to_byte(float value)
{
  return
    (value > 254.0f) ? 254 : // Sync byte is 255 so avoid that
    ((value < 0.0f) ? 0 :
    (unsigned char)(value));
}

void send_empty_packet()
{
  memset(packet, 0, PACKET_MAX_SIZE); 
  packet[0] = PACKET_SYNC;
  packet[1] = PACKET_VER;
  
  serial_port.write(packet, 16);
}

SCSAPI_VOID telemetry_frame_end(const scs_event_t /*event*/, const void *const /*event_info*/, const scs_context_t /*context*/)
{
  if (!serial_port.is_valid())
    return;
    
  const unsigned long now = GetTickCount();
  const unsigned long diff = now - last_update;
  if (diff < 50)
    return;
    
  last_update = now;
  
  const float speed_mph = telemetry.speed * METERS_PER_SEC_TO_MILES_PER_HOUR;
  const float speed_kph = telemetry.speed * METERS_PER_SEC_TO_KM_PER_HOUR;
  
  const float fuel_ratio = telemetry.fuel /  telemetry.fuel_capacity;
  

  
  unsigned idx = 0;
  
  memset(packet, 0, PACKET_MAX_SIZE);
  
#define PUT_BYTE(X) packet[idx++] = (unsigned char)(X)
#define PUT_BYTE_FLOAT(X) packet[idx++] = (unsigned char)(float_to_byte(X))
#define GETFLTOPT(X) (option_file.get_option_float(X, 1.0f))
  
  // Packet header
  PUT_BYTE(PACKET_SYNC);
  PUT_BYTE(PACKET_VER);
  
  // Convert data for output by servos
  // Adjust the constant values to map to servo range
  
  PUT_BYTE_FLOAT(fabs(telemetry.speed) *        GETFLTOPT("factor_speed")); // Absolute value for when reversing
  PUT_BYTE_FLOAT(telemetry.engine_rpm *         GETFLTOPT("factor_engine_rpm"));
  PUT_BYTE_FLOAT(telemetry.brake_air_pressure * GETFLTOPT("factor_brake_air_pressure"));
  PUT_BYTE_FLOAT(telemetry.brake_temperature *  GETFLTOPT("factor_brake_temperature"));
  PUT_BYTE_FLOAT(fuel_ratio *                   GETFLTOPT("factor_fuel_ratio")); // Fuel level
  PUT_BYTE_FLOAT(telemetry.oil_pressure *       GETFLTOPT("factor_oil_pressure"));
  PUT_BYTE_FLOAT(telemetry.oil_temperature *    GETFLTOPT("factor_oil_temperature"));
  PUT_BYTE_FLOAT(telemetry.water_temperature *  GETFLTOPT("factor_water_temperature"));
  PUT_BYTE_FLOAT(telemetry.battery_voltage *    GETFLTOPT("factor_battery_voltage"));
  
  
  // Pack data for LEDs into bytes
  
  // Truck lights
  PUT_BYTE(PACKBOOL(
    0, telemetry.light_parking,
    telemetry.light_lblinker, telemetry.light_rblinker,
    telemetry.light_low_beam, telemetry.light_high_beam,
    telemetry.light_brake, telemetry.light_reverse));
    
  // Warning lights
  PUT_BYTE(PACKBOOL(
    telemetry.parking_brake, telemetry.motor_brake,
    telemetry.brake_air_pressure_warning, telemetry.brake_air_pressure_emergency,
    telemetry.fuel_warning, telemetry.battery_voltage_warning,
    telemetry.oil_pressure_warning, telemetry.water_temperature_warning));
  
  // Enabled flags
  PUT_BYTE(PACKBOOL(
    0,0,0,0,0, 0,
    telemetry.electric_enabled, telemetry.engine_enabled));
    

  // Build string for LCD display
    
  std::stringstream ss;
  
  ss.width(3);
  ss << abs(int(speed_mph));
  ss << " MPH";
  
  ss << "   G  ";
  
  if (telemetry.engine_gear > 0)
  {
    ss << "D";
    ss.width(2);
    ss << telemetry.engine_gear;
  }
  else if (telemetry.engine_gear == 0)
  {
    ss << "N  ";
  }
  else
  {
    ss << "R";
    ss.width(2);
    ss << abs(telemetry.engine_gear);
  }
  
  ss << "\n";
  
  ss.width(3);
  ss << abs(int(speed_kph));
  ss << " KPH";
  
  ss << "   F ";
  
  ss.width(3);
  ss << int(fuel_ratio * 100.0f) << "%";
  
  
  ss.width(3);
  ss << abs(int(speed_kph));
  
  std::string display_str(ss.str());
  
  // Write length of string
  PUT_BYTE(display_str.size());
  // Followed by string
  display_str.copy((char*)&packet[idx], PACKET_MAX_SIZE - idx);
  idx += display_str.size();
  
  
  serial_port.write(packet, idx);
}






SCSAPI_VOID telemetry_store_float(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_float);
	assert(context);
	*static_cast<float *>(context) = value->value_float.value;
}

SCSAPI_VOID telemetry_store_bool(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_bool);
	assert(context);
	*static_cast<bool *>(context) = (value->value_bool.value != 0);
}

SCSAPI_VOID telemetry_store_s32(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_s32);
	assert(context);
	*static_cast<int *>(context) = value->value_s32.value;
}

SCSAPI_VOID telemetry_configuration(const scs_event_t /*event*/, const void *const event_info, const scs_context_t /*context*/)
{
  const struct scs_telemetry_configuration_t *const info = static_cast<const scs_telemetry_configuration_t *>(event_info);
  
  for (const scs_named_value_t *current = info->attributes; current->name; ++current)
  {
  
#define GET_CONFIG(PROPERTY, TYPE) \
  if (strcmp(SCS_TELEMETRY_CONFIG_ATTRIBUTE_ ## PROPERTY, current->name) == 0) \
  { telemetry.PROPERTY = current->value.value_ ## TYPE.value; }
  
    GET_CONFIG(fuel_capacity, float);
    
  }
}

void get_cwd(std::string& str)
{
  char buffer[256];
  GetCurrentDirectory(256, buffer);
  str.assign(buffer);
}

SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t *const params)
{
	if (version != SCS_TELEMETRY_VERSION_1_00) {
		return SCS_RESULT_unsupported;
	}

	const scs_telemetry_init_params_v100_t *const version_params = static_cast<const scs_telemetry_init_params_v100_t *>(params);
  game_log = version_params->common.log;
  
  game_log(SCS_LOG_TYPE_message, "Plugin initialising");
  
  std::string cwd;
  get_cwd(cwd);
  
  game_log(SCS_LOG_TYPE_message, (std::string("Plugin CWD: ") + cwd).c_str());
  
  std::string option_filepath(cwd + "\\plugins\\dash_plugin.txt");
  if (!option_file.read_file(option_filepath))
  {
    game_log(SCS_LOG_TYPE_error, (std::string("Error reading settings file: ") + option_filepath).c_str());
    return SCS_RESULT_generic_error;
  }
  
  const std::string com_port = option_file.get_option_string("comport", "COM3");
  game_log(SCS_LOG_TYPE_message, (std::string("Using serial port: ") + com_port).c_str());
  
  // Open COM port
  std::string errmsg;
  if (!serial_port.open(com_port, errmsg))
  {
    game_log(SCS_LOG_TYPE_error, errmsg.c_str());
    return SCS_RESULT_generic_error;
  }
    
  send_empty_packet();
  
  // Register for in game events
  bool registered =
    (version_params->register_for_event(
      SCS_TELEMETRY_EVENT_frame_end, telemetry_frame_end, NULL) == SCS_RESULT_ok) &&
    (version_params->register_for_event(
      SCS_TELEMETRY_EVENT_configuration, telemetry_configuration, NULL) == SCS_RESULT_ok);
  
  // Register for truck channels
  
#define REG_CHAN(CHANNEL, TYPE) \
  registered &= (version_params->register_for_channel( \
  SCS_TELEMETRY_TRUCK_CHANNEL_ ## CHANNEL, SCS_U32_NIL, SCS_VALUE_TYPE_ ## TYPE, \
  SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_ ## TYPE, &telemetry.CHANNEL) == SCS_RESULT_ok)
  
  REG_CHAN(speed,                         float);
  REG_CHAN(engine_rpm,                    float);
  REG_CHAN(engine_gear,                   s32);
  REG_CHAN(parking_brake,                 bool);
  REG_CHAN(motor_brake,                   bool);
  REG_CHAN(brake_air_pressure,            float);
  REG_CHAN(brake_air_pressure_warning,    bool);
  REG_CHAN(brake_air_pressure_emergency,  bool);
  REG_CHAN(brake_temperature,             float);
  REG_CHAN(fuel,                          float);
  REG_CHAN(fuel_warning,                  bool);
  REG_CHAN(fuel_average_consumption,      float);
  REG_CHAN(oil_pressure,                  float);
  REG_CHAN(oil_pressure_warning,          bool);
  REG_CHAN(oil_temperature,               float);
  REG_CHAN(water_temperature,             float);
  REG_CHAN(water_temperature_warning,     bool);
  REG_CHAN(battery_voltage,               float);
  REG_CHAN(battery_voltage_warning,       bool);
  REG_CHAN(electric_enabled,              bool);
  REG_CHAN(engine_enabled,                bool);
  REG_CHAN(light_lblinker,                bool);
  REG_CHAN(light_rblinker,                bool);
  REG_CHAN(light_parking,                 bool);
  REG_CHAN(light_low_beam,                bool);
  REG_CHAN(light_high_beam,               bool);
  REG_CHAN(light_brake,                   bool);
  REG_CHAN(light_reverse,                 bool);
  REG_CHAN(odometer,                      float);
  

  if (!registered)
  {
    game_log(SCS_LOG_TYPE_error, "Unable to register callbacks");
		return SCS_RESULT_generic_error;
  }
  
  memset(&telemetry, 0, sizeof(telemetry));
  
  return SCS_RESULT_ok;
}

SCSAPI_VOID scs_telemetry_shutdown(void)
{
  send_empty_packet();
  serial_port.close();

  game_log(SCS_LOG_TYPE_message, "Plugin shutdown");
	game_log = NULL;
}

BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason_for_call, LPVOID /*reseved*/)
{
  if (reason_for_call == DLL_PROCESS_DETACH)
  {
		serial_port.close();
	}
	return TRUE;
}

