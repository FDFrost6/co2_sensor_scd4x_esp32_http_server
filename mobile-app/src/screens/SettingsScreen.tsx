import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TextInput,
  TouchableOpacity,
  Alert,
  ActivityIndicator,
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';
import { Ionicons } from '@expo/vector-icons';
import { useDeviceStore } from '../store/deviceStore';
import { colors } from '../theme/colors';

export function SettingsScreen() {
  const {
    devices,
    currentDevice,
    deviceInfo,
    isConnected,
    isLoading,
    error,
    connectToDevice,
    discoverDevices,
    setLightSchedule,
  } = useDeviceStore();

  const [ipAddress, setIpAddress] = useState('growcontroller.local');
  const [lightOnHour, setLightOnHour] = useState(deviceInfo?.light_on_hour?.toString() || '6');
  const [lightOffHour, setLightOffHour] = useState(deviceInfo?.light_off_hour?.toString() || '22');

  const handleConnect = async () => {
    const success = await connectToDevice(ipAddress);
    if (success) {
      Alert.alert('Connected', 'Successfully connected to GrowController');
    } else {
      Alert.alert('Connection Failed', 'Could not connect to device. Check the IP address and ensure the device is powered on.');
    }
  };

  const handleDiscover = async () => {
    await discoverDevices();
    if (devices.length === 0) {
      Alert.alert('No Devices Found', 'Make sure your GrowController is powered on and connected to the same network.');
    }
  };

  const handleSaveLightSchedule = async () => {
    const onHour = parseInt(lightOnHour, 10);
    const offHour = parseInt(lightOffHour, 10);
    
    if (isNaN(onHour) || isNaN(offHour) || onHour < 0 || onHour > 23 || offHour < 0 || offHour > 23) {
      Alert.alert('Invalid Hours', 'Please enter valid hours between 0 and 23');
      return;
    }
    
    await setLightSchedule(onHour, offHour);
    Alert.alert('Saved', 'Light schedule updated');
  };

  const formatUptime = (ms: number) => {
    const hours = Math.floor(ms / 3600000);
    const days = Math.floor(hours / 24);
    const remainingHours = hours % 24;
    if (days > 0) return `${days}d ${remainingHours}h`;
    return `${hours}h`;
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar style="light" />
      <ScrollView style={styles.scrollView} contentContainerStyle={styles.scrollContent}>
        <Text style={styles.title}>SETTINGS</Text>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>DEVICE CONNECTION</Text>
          
          <View style={styles.inputGroup}>
            <Text style={styles.label}>Device Address</Text>
            <View style={styles.inputRow}>
              <TextInput
                style={styles.input}
                value={ipAddress}
                onChangeText={setIpAddress}
                placeholder="growcontroller.local"
                placeholderTextColor={colors.textMuted}
                autoCapitalize="none"
                autoCorrect={false}
              />
              <TouchableOpacity
                style={[styles.button, styles.connectButton]}
                onPress={handleConnect}
                disabled={isLoading}
              >
                {isLoading ? (
                  <ActivityIndicator size="small" color={colors.background} />
                ) : (
                  <Ionicons name="link" size={20} color={colors.background} />
                )}
              </TouchableOpacity>
            </View>
          </View>

          <TouchableOpacity
            style={styles.discoverButton}
            onPress={handleDiscover}
            disabled={isLoading}
          >
            <Ionicons name="search" size={18} color={colors.primary} />
            <Text style={styles.discoverButtonText}>Discover Devices</Text>
          </TouchableOpacity>

          {devices.length > 0 && (
            <View style={styles.deviceList}>
              <Text style={styles.subLabel}>Found Devices:</Text>
              {devices.map((device) => (
                <TouchableOpacity
                  key={device.id}
                  style={[
                    styles.deviceItem,
                    currentDevice?.id === device.id && styles.deviceItemActive,
                  ]}
                  onPress={() => connectToDevice(device.ipAddress)}
                >
                  <View style={styles.deviceItemLeft}>
                    <Ionicons
                      name="hardware-chip"
                      size={20}
                      color={currentDevice?.id === device.id ? colors.primary : colors.textSecondary}
                    />
                    <View style={styles.deviceItemText}>
                      <Text style={styles.deviceName}>{device.name}</Text>
                      <Text style={styles.deviceIp}>{device.ipAddress}</Text>
                    </View>
                  </View>
                  {currentDevice?.id === device.id && (
                    <Ionicons name="checkmark-circle" size={20} color={colors.primary} />
                  )}
                </TouchableOpacity>
              ))}
            </View>
          )}

          {error && <Text style={styles.error}>{error}</Text>}
        </View>

        {isConnected && deviceInfo && (
          <>
            <View style={styles.section}>
              <Text style={styles.sectionTitle}>LIGHT SCHEDULE</Text>
              
              <View style={styles.scheduleRow}>
                <View style={styles.scheduleInput}>
                  <Text style={styles.label}>Lights On</Text>
                  <TextInput
                    style={styles.hourInput}
                    value={lightOnHour}
                    onChangeText={setLightOnHour}
                    keyboardType="numeric"
                    maxLength={2}
                    placeholder="6"
                    placeholderTextColor={colors.textMuted}
                  />
                  <Text style={styles.hourLabel}>:00</Text>
                </View>
                
                <View style={styles.scheduleInput}>
                  <Text style={styles.label}>Lights Off</Text>
                  <TextInput
                    style={styles.hourInput}
                    value={lightOffHour}
                    onChangeText={setLightOffHour}
                    keyboardType="numeric"
                    maxLength={2}
                    placeholder="22"
                    placeholderTextColor={colors.textMuted}
                  />
                  <Text style={styles.hourLabel}>:00</Text>
                </View>
              </View>

              <TouchableOpacity style={styles.saveButton} onPress={handleSaveLightSchedule}>
                <Ionicons name="save" size={18} color={colors.background} />
                <Text style={styles.saveButtonText}>Save Schedule</Text>
              </TouchableOpacity>
            </View>

            <View style={styles.section}>
              <Text style={styles.sectionTitle}>DEVICE INFO</Text>
              
              <View style={styles.infoGrid}>
                <InfoRow label="Hostname" value={deviceInfo.hostname} />
                <InfoRow label="IP Address" value={deviceInfo.ip_address} />
                <InfoRow label="MAC Address" value={deviceInfo.mac_address} />
                <InfoRow label="Firmware" value={`v${deviceInfo.firmware_version}`} />
                <InfoRow label="Sensor" value={deviceInfo.sensor_type} />
                <InfoRow label="Uptime" value={formatUptime(deviceInfo.uptime_ms)} />
                <InfoRow label="Signal" value={`${deviceInfo.rssi} dBm`} />
                <InfoRow label="Free Memory" value={`${Math.round(deviceInfo.free_heap / 1024)} KB`} />
                <InfoRow label="Time Synced" value={deviceInfo.time_synced ? 'Yes' : 'No'} />
              </View>
            </View>
          </>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

function InfoRow({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  scrollView: {
    flex: 1,
  },
  scrollContent: {
    padding: 16,
    paddingBottom: 32,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: colors.text,
    letterSpacing: 2,
    marginBottom: 24,
  },
  section: {
    backgroundColor: colors.surface,
    borderRadius: 16,
    padding: 20,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: colors.border,
  },
  sectionTitle: {
    color: colors.textSecondary,
    fontSize: 12,
    fontWeight: '600',
    letterSpacing: 1,
    marginBottom: 16,
  },
  inputGroup: {
    marginBottom: 16,
  },
  label: {
    color: colors.textSecondary,
    fontSize: 12,
    marginBottom: 8,
  },
  subLabel: {
    color: colors.textMuted,
    fontSize: 11,
    marginBottom: 8,
  },
  inputRow: {
    flexDirection: 'row',
    gap: 12,
  },
  input: {
    flex: 1,
    backgroundColor: colors.surfaceLight,
    borderRadius: 8,
    paddingHorizontal: 16,
    paddingVertical: 12,
    color: colors.text,
    fontSize: 16,
  },
  button: {
    width: 48,
    height: 48,
    borderRadius: 8,
    alignItems: 'center',
    justifyContent: 'center',
  },
  connectButton: {
    backgroundColor: colors.primary,
  },
  discoverButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 8,
    paddingVertical: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.primary,
  },
  discoverButtonText: {
    color: colors.primary,
    fontWeight: '600',
  },
  deviceList: {
    marginTop: 16,
  },
  deviceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: colors.surfaceLight,
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  deviceItemActive: {
    borderWidth: 1,
    borderColor: colors.primary,
  },
  deviceItemLeft: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  deviceItemText: {
    marginLeft: 12,
  },
  deviceName: {
    color: colors.text,
    fontWeight: '600',
  },
  deviceIp: {
    color: colors.textMuted,
    fontSize: 12,
    marginTop: 2,
  },
  error: {
    color: colors.danger,
    fontSize: 12,
    marginTop: 12,
    textAlign: 'center',
  },
  scheduleRow: {
    flexDirection: 'row',
    gap: 16,
    marginBottom: 16,
  },
  scheduleInput: {
    flex: 1,
  },
  hourInput: {
    backgroundColor: colors.surfaceLight,
    borderRadius: 8,
    paddingHorizontal: 16,
    paddingVertical: 12,
    color: colors.text,
    fontSize: 24,
    fontWeight: 'bold',
    textAlign: 'center',
  },
  hourLabel: {
    color: colors.textMuted,
    fontSize: 14,
    textAlign: 'center',
    marginTop: 4,
  },
  saveButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 8,
    backgroundColor: colors.primary,
    paddingVertical: 14,
    borderRadius: 8,
  },
  saveButtonText: {
    color: colors.background,
    fontWeight: 'bold',
  },
  infoGrid: {
    gap: 8,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: colors.border,
  },
  infoLabel: {
    color: colors.textSecondary,
    fontSize: 14,
  },
  infoValue: {
    color: colors.text,
    fontSize: 14,
    fontWeight: '500',
  },
});
