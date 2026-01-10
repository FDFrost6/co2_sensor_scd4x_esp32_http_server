import { create } from 'zustand';
import { SensorStatus, DeviceInfo, HistoricalDataPoint, Device } from '../types';
import { api } from '../services/api';

interface DeviceState {
  devices: Device[];
  currentDevice: Device | null;
  status: SensorStatus | null;
  deviceInfo: DeviceInfo | null;
  historicalData: HistoricalDataPoint[];
  isLoading: boolean;
  isConnected: boolean;
  error: string | null;
  lastUpdate: number;

  setCurrentDevice: (device: Device) => void;
  addDevice: (device: Device) => void;
  removeDevice: (deviceId: string) => void;
  fetchStatus: () => Promise<void>;
  fetchDeviceInfo: () => Promise<void>;
  fetchHistoricalData: () => Promise<void>;
  setGrowStage: (stage: 'veg' | 'flower') => Promise<void>;
  startPlantTimer: () => Promise<void>;
  stopPlantTimer: () => Promise<void>;
  resetPlantTimer: () => Promise<void>;
  setLightSchedule: (onHour: number, offHour: number) => Promise<void>;
  discoverDevices: () => Promise<void>;
  connectToDevice: (ipAddress: string) => Promise<boolean>;
}

export const useDeviceStore = create<DeviceState>((set, get) => ({
  devices: [],
  currentDevice: null,
  status: null,
  deviceInfo: null,
  historicalData: [],
  isLoading: false,
  isConnected: false,
  error: null,
  lastUpdate: 0,

  setCurrentDevice: (device) => {
    api.setBaseUrl(device.ipAddress);
    set({ currentDevice: device, isConnected: true, error: null });
  },

  addDevice: (device) => {
    set((state) => ({
      devices: [...state.devices.filter((d) => d.id !== device.id), device],
    }));
  },

  removeDevice: (deviceId) => {
    set((state) => ({
      devices: state.devices.filter((d) => d.id !== deviceId),
    }));
  },

  fetchStatus: async () => {
    if (!get().isConnected) return;

    try {
      const status = await api.getStatus();
      set({ status, lastUpdate: Date.now(), error: null });
    } catch (err) {
      set({ error: 'Failed to fetch status', isConnected: false });
    }
  },

  fetchDeviceInfo: async () => {
    if (!get().isConnected) return;

    try {
      set({ isLoading: true });
      const deviceInfo = await api.getDeviceInfo();
      set({ deviceInfo, isLoading: false, error: null });
    } catch (err) {
      set({ error: 'Failed to fetch device info', isLoading: false });
    }
  },

  fetchHistoricalData: async () => {
    if (!get().isConnected) return;

    try {
      set({ isLoading: true });
      const historicalData = await api.getHistoricalData();
      set({ historicalData, isLoading: false, error: null });
    } catch (err) {
      set({ error: 'Failed to fetch historical data', isLoading: false });
    }
  },

  setGrowStage: async (stage) => {
    try {
      await api.setGrowStage(stage);
      await get().fetchStatus();
    } catch (err) {
      set({ error: 'Failed to set grow stage' });
    }
  },

  startPlantTimer: async () => {
    try {
      await api.startPlantTimer();
      await get().fetchStatus();
    } catch (err) {
      set({ error: 'Failed to start plant timer' });
    }
  },

  stopPlantTimer: async () => {
    try {
      await api.stopPlantTimer();
      await get().fetchStatus();
    } catch (err) {
      set({ error: 'Failed to stop plant timer' });
    }
  },

  resetPlantTimer: async () => {
    try {
      await api.resetPlantTimer();
      await get().fetchStatus();
    } catch (err) {
      set({ error: 'Failed to reset plant timer' });
    }
  },

  setLightSchedule: async (onHour, offHour) => {
    try {
      await api.setLightSchedule(onHour, offHour);
      await get().fetchDeviceInfo();
    } catch (err) {
      set({ error: 'Failed to set light schedule' });
    }
  },

  discoverDevices: async () => {
    set({ isLoading: true });
    try {
      const ips = await api.discoverDevices();
      const devices: Device[] = [];

      for (const ip of ips) {
        try {
          api.setBaseUrl(ip);
          const info = await api.getDeviceInfo();
          devices.push({
            id: info.mac_address,
            name: info.device_name,
            ipAddress: ip,
            hostname: info.hostname,
            lastSeen: Date.now(),
            isOnline: true,
          });
        } catch {
          continue;
        }
      }

      set({ devices, isLoading: false });
    } catch (err) {
      set({ error: 'Failed to discover devices', isLoading: false });
    }
  },

  connectToDevice: async (ipAddress) => {
    set({ isLoading: true, error: null });
    try {
      const isOnline = await api.checkConnection(ipAddress);
      if (!isOnline) {
        set({ error: 'Device not reachable', isLoading: false });
        return false;
      }

      api.setBaseUrl(ipAddress);
      const deviceInfo = await api.getDeviceInfo();
      const status = await api.getStatus();

      const device: Device = {
        id: deviceInfo.mac_address,
        name: deviceInfo.device_name,
        ipAddress,
        hostname: deviceInfo.hostname,
        lastSeen: Date.now(),
        isOnline: true,
      };

      set({
        currentDevice: device,
        deviceInfo,
        status,
        isConnected: true,
        isLoading: false,
        error: null,
      });

      get().addDevice(device);
      return true;
    } catch (err) {
      set({
        error: 'Failed to connect to device',
        isLoading: false,
        isConnected: false,
      });
      return false;
    }
  },
}));
