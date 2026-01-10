import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../theme/colors';

interface MetricCardProps {
  title: string;
  value: number | string;
  unit: string;
  color?: string;
  subtitle?: string;
}

export function MetricCard({ title, value, unit, color = colors.primary, subtitle }: MetricCardProps) {
  return (
    <View style={styles.card}>
      <Text style={styles.title}>{title}</Text>
      <Text style={[styles.value, { color, textShadowColor: color }]}>
        {typeof value === 'number' ? value.toFixed(1) : value}
      </Text>
      <Text style={[styles.unit, { color }]}>{unit}</Text>
      {subtitle && <Text style={styles.subtitle}>{subtitle}</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.surface,
    borderRadius: 16,
    padding: 20,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: colors.border,
    minWidth: '45%',
    marginBottom: 16,
  },
  title: {
    color: colors.textSecondary,
    fontSize: 12,
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginBottom: 8,
  },
  value: {
    fontSize: 42,
    fontWeight: 'bold',
    textShadowOffset: { width: 0, height: 0 },
    textShadowRadius: 10,
  },
  unit: {
    fontSize: 16,
    fontWeight: '600',
    marginTop: 4,
  },
  subtitle: {
    color: colors.textMuted,
    fontSize: 11,
    marginTop: 8,
  },
});
