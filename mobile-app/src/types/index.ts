export interface SensorStatus {
  temperature_c: number;
  ambient_temperature_c: number;
  humidity_percent: number;
  co2_ppm: number;
  vpd_kpa: number;
  vpd_status: 'too_low' | 'optimal' | 'too_high';
  vpd_min: number;
  vpd_max: number;
  grow_stage: 'veg' | 'flower';
  plant_timer_active: boolean;
  plant_age_days: number;
  light_on: boolean;
  battery_voltage: number;
  firmware_version: string;
}

export interface DeviceInfo {
  device_name: string;
  firmware_version: string;
  hostname: string;
  ip_address: string;
  mac_address: string;
  rssi: number;
  uptime_ms: number;
  free_heap: number;
  sensor_type: string;
  plant_timer_active: boolean;
  plant_age_days: number;
  light_on_hour: number;
  light_off_hour: number;
  time_synced: boolean;
}

export interface HistoricalDataPoint {
  t: number;
  temp: number;
  hum: number;
  co2: number;
  vpd: number;
}

export interface Device {
  id: string;
  name: string;
  ipAddress: string;
  hostname: string;
  lastSeen: number;
  isOnline: boolean;
}

export type GrowStage = 'veg' | 'flower';
export type VpdStatus = 'too_low' | 'optimal' | 'too_high';
