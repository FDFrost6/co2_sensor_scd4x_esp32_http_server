import React, { useCallback } from 'react';
import { View, Text, StyleSheet, ScrollView, RefreshControl } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';
import { useDeviceStore } from '../store/deviceStore';
import { usePolling } from '../hooks/usePolling';
import { MetricCard } from '../components/MetricCard';
import { VpdIndicator } from '../components/VpdIndicator';
import { PlantStatus } from '../components/PlantStatus';
import { colors } from '../theme/colors';

export function DashboardScreen() {
  const {
    status,
    deviceInfo,
    isConnected,
    isLoading,
    lastUpdate,
    currentDevice,
    fetchStatus,
    setGrowStage,
    startPlantTimer,
    stopPlantTimer,
    resetPlantTimer,
  } = useDeviceStore();

  usePolling(fetchStatus, 5000, isConnected);

  const onRefresh = useCallback(async () => {
    await fetchStatus();
  }, [fetchStatus]);

  const formatLastUpdate = () => {
    if (!lastUpdate) return 'Never';
    const seconds = Math.floor((Date.now() - lastUpdate) / 1000);
    if (seconds < 5) return 'Just now';
    if (seconds < 60) return `${seconds}s ago`;
    return `${Math.floor(seconds / 60)}m ago`;
  };

  if (!isConnected || !status) {
    return (
      <SafeAreaView style={styles.container}>
        <StatusBar style="light" />
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedIcon}>📡</Text>
          <Text style={styles.disconnectedTitle}>Not Connected</Text>
          <Text style={styles.disconnectedText}>
            Go to Settings to connect to your GrowController
          </Text>
        </View>
      </SafeAreaView>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar style="light" />
      <ScrollView
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
        refreshControl={
          <RefreshControl
            refreshing={isLoading}
            onRefresh={onRefresh}
            tintColor={colors.primary}
            colors={[colors.primary]}
          />
        }
      >
        <View style={styles.header}>
          <Text style={styles.title}>GROW MONITOR</Text>
          <View style={styles.headerRight}>
            <View style={[styles.statusDot, { backgroundColor: colors.primary }]} />
            <Text style={styles.lastUpdate}>{formatLastUpdate()}</Text>
          </View>
        </View>

        <View style={styles.metricsGrid}>
          <MetricCard
            title="Temperature"
            value={status.temperature_c}
            unit="°C"
            color={colors.temperature}
          />
          <MetricCard
            title="Humidity"
            value={status.humidity_percent}
            unit="%"
            color={colors.humidity}
          />
          <MetricCard
            title="CO2"
            value={Math.round(status.co2_ppm)}
            unit="PPM"
            color={colors.co2}
          />
          <MetricCard
            title="VPD"
            value={status.vpd_kpa}
            unit="kPa"
            color={colors.vpd}
            subtitle={`Target: ${status.vpd_min?.toFixed(1) || '0.8'}-${status.vpd_max?.toFixed(1) || '1.2'}`}
          />
        </View>

        <VpdIndicator
          vpd={status.vpd_kpa}
          status={status.vpd_status}
          minVpd={status.vpd_min || 0.8}
          maxVpd={status.vpd_max || 1.2}
          growStage={status.grow_stage}
        />

        <PlantStatus
          growStage={status.grow_stage}
          plantTimerActive={status.plant_timer_active}
          plantAgeDays={status.plant_age_days}
          lightOn={status.light_on}
          onStageChange={setGrowStage}
          onTimerStart={startPlantTimer}
          onTimerStop={stopPlantTimer}
          onTimerReset={resetPlantTimer}
        />

        <View style={styles.deviceInfo}>
          <Text style={styles.deviceInfoText}>
            {currentDevice?.name || 'GrowController'} • v{status.firmware_version}
          </Text>
          <Text style={styles.deviceInfoText}>
            {currentDevice?.ipAddress || 'Unknown IP'}
          </Text>
        </View>
      </ScrollView>
    </SafeAreaView>
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
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: colors.text,
    letterSpacing: 2,
  },
  headerRight: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  lastUpdate: {
    color: colors.textMuted,
    fontSize: 12,
  },
  metricsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  disconnected: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 32,
  },
  disconnectedIcon: {
    fontSize: 64,
    marginBottom: 16,
  },
  disconnectedTitle: {
    fontSize: 24,
    fontWeight: 'bold',
    color: colors.text,
    marginBottom: 8,
  },
  disconnectedText: {
    fontSize: 16,
    color: colors.textSecondary,
    textAlign: 'center',
  },
  deviceInfo: {
    marginTop: 24,
    alignItems: 'center',
    paddingBottom: 32,
  },
  deviceInfoText: {
    color: colors.textMuted,
    fontSize: 12,
    marginVertical: 2,
  },
});
