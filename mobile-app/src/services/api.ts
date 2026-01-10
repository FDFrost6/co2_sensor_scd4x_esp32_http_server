import { SensorStatus, DeviceInfo, HistoricalDataPoint } from '../types';

const DEFAULT_TIMEOUT = 5000;

class GrowControllerAPI {
  private baseUrl: string = '';

  setBaseUrl(ipAddress: string) {
    this.baseUrl = `http://${ipAddress}`;
  }

  getBaseUrl(): string {
    return this.baseUrl;
  }

  private async fetchWithTimeout<T>(
    endpoint: string,
    timeout: number = DEFAULT_TIMEOUT
  ): Promise<T> {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeout);

    try {
      const response = await fetch(`${this.baseUrl}${endpoint}`, {
        signal: controller.signal,
        headers: {
          'Accept': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      return await response.json();
    } finally {
      clearTimeout(timeoutId);
    }
  }

  async getStatus(): Promise<SensorStatus> {
    return this.fetchWithTimeout<SensorStatus>('/status');
  }

  async getDeviceInfo(): Promise<DeviceInfo> {
    return this.fetchWithTimeout<DeviceInfo>('/api/info');
  }

  async getHistoricalData(): Promise<HistoricalDataPoint[]> {
    return this.fetchWithTimeout<HistoricalDataPoint[]>('/data', 10000);
  }

  async setGrowStage(stage: 'veg' | 'flower'): Promise<void> {
    await fetch(`${this.baseUrl}/stage/${stage}`);
  }

  async startPlantTimer(): Promise<void> {
    await fetch(`${this.baseUrl}/timer/start`);
  }

  async stopPlantTimer(): Promise<void> {
    await fetch(`${this.baseUrl}/timer/stop`);
  }

  async resetPlantTimer(): Promise<void> {
    await fetch(`${this.baseUrl}/timer/reset`);
  }

  async setLightSchedule(onHour: number, offHour: number): Promise<void> {
    await fetch(`${this.baseUrl}/light/set?on=${onHour}&off=${offHour}`);
  }

  async checkConnection(ipAddress: string): Promise<boolean> {
    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 3000);

      const response = await fetch(`http://${ipAddress}/api/info`, {
        signal: controller.signal,
      });

      clearTimeout(timeoutId);
      return response.ok;
    } catch {
      return false;
    }
  }

  async discoverDevices(): Promise<string[]> {
    const discoveredIPs: string[] = [];
    const commonIPs = [
      'growcontroller.local',
      '192.168.1.100',
      '192.168.1.101',
      '192.168.0.100',
      '192.168.0.101',
    ];

    const checks = commonIPs.map(async (ip) => {
      const isOnline = await this.checkConnection(ip);
      if (isOnline) {
        discoveredIPs.push(ip);
      }
    });

    await Promise.all(checks);
    return discoveredIPs;
  }
}

export const api = new GrowControllerAPI();
