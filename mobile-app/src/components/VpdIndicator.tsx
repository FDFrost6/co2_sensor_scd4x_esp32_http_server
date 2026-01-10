import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { colors } from '../theme/colors';
import { VpdStatus } from '../types';

interface VpdIndicatorProps {
  vpd: number;
  status: VpdStatus;
  minVpd: number;
  maxVpd: number;
  growStage: 'veg' | 'flower';
}

export function VpdIndicator({ vpd, status, minVpd, maxVpd, growStage }: VpdIndicatorProps) {
  const getStatusConfig = () => {
    switch (status) {
      case 'optimal':
        return {
          color: colors.vpdOptimal,
          icon: 'checkmark-circle' as const,
          message: 'VPD OPTIMAL',
          action: 'Conditions are perfect for plant growth',
        };
      case 'too_low':
        return {
          color: colors.vpdTooLow,
          icon: 'arrow-down-circle' as const,
          message: 'VPD TOO LOW',
          action: 'Increase temperature or decrease humidity',
        };
      case 'too_high':
        return {
          color: colors.vpdTooHigh,
          icon: 'arrow-up-circle' as const,
          message: 'VPD TOO HIGH',
          action: 'Decrease temperature or increase humidity',
        };
    }
  };

  const config = getStatusConfig();
  const progress = Math.min(Math.max((vpd - 0.4) / 1.4, 0), 1);

  return (
    <View style={[styles.container, { borderColor: config.color }]}>
      <View style={styles.header}>
        <Text style={styles.title}>VPD STATUS</Text>
        <Text style={[styles.stage, { color: growStage === 'veg' ? colors.secondary : '#ff6b9d' }]}>
          {growStage.toUpperCase()}
        </Text>
      </View>

      <View style={styles.valueContainer}>
        <Text style={[styles.value, { color: config.color }]}>{vpd.toFixed(2)}</Text>
        <Text style={styles.unit}>kPa</Text>
      </View>

      <View style={styles.rangeBar}>
        <View style={[styles.rangeFill, { width: `${progress * 100}%`, backgroundColor: config.color }]} />
        <View style={[styles.rangeMarker, { left: `${((minVpd - 0.4) / 1.4) * 100}%` }]} />
        <View style={[styles.rangeMarker, { left: `${((maxVpd - 0.4) / 1.4) * 100}%` }]} />
      </View>

      <Text style={styles.rangeText}>
        Optimal: {minVpd.toFixed(1)} - {maxVpd.toFixed(1)} kPa
      </Text>

      <View style={styles.statusContainer}>
        <Ionicons name={config.icon} size={28} color={config.color} />
        <View style={styles.statusText}>
          <Text style={[styles.statusMessage, { color: config.color }]}>{config.message}</Text>
          <Text style={styles.statusAction}>{config.action}</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: colors.surface,
    borderRadius: 16,
    padding: 20,
    borderWidth: 2,
    marginVertical: 16,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  title: {
    color: colors.textSecondary,
    fontSize: 12,
    fontWeight: '600',
    letterSpacing: 1,
  },
  stage: {
    fontSize: 12,
    fontWeight: 'bold',
    letterSpacing: 1,
  },
  valueContainer: {
    flexDirection: 'row',
    alignItems: 'baseline',
    justifyContent: 'center',
    marginBottom: 16,
  },
  value: {
    fontSize: 56,
    fontWeight: 'bold',
  },
  unit: {
    fontSize: 20,
    color: colors.textSecondary,
    marginLeft: 8,
  },
  rangeBar: {
    height: 8,
    backgroundColor: colors.surfaceLight,
    borderRadius: 4,
    marginBottom: 8,
    position: 'relative',
    overflow: 'hidden',
  },
  rangeFill: {
    height: '100%',
    borderRadius: 4,
  },
  rangeMarker: {
    position: 'absolute',
    top: -2,
    width: 2,
    height: 12,
    backgroundColor: colors.text,
    opacity: 0.5,
  },
  rangeText: {
    color: colors.textMuted,
    fontSize: 12,
    textAlign: 'center',
    marginBottom: 16,
  },
  statusContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: colors.surfaceLight,
    borderRadius: 12,
    padding: 16,
  },
  statusText: {
    marginLeft: 12,
    flex: 1,
  },
  statusMessage: {
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  statusAction: {
    fontSize: 12,
    color: colors.textSecondary,
  },
});
