import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { colors } from '../theme/colors';

interface PlantStatusProps {
  growStage: 'veg' | 'flower';
  plantTimerActive: boolean;
  plantAgeDays: number;
  lightOn: boolean;
  onStageChange: (stage: 'veg' | 'flower') => void;
  onTimerStart: () => void;
  onTimerStop: () => void;
  onTimerReset: () => void;
}

export function PlantStatus({
  growStage,
  plantTimerActive,
  plantAgeDays,
  lightOn,
  onStageChange,
  onTimerStart,
  onTimerStop,
  onTimerReset,
}: PlantStatusProps) {
  const formatAge = (days: number) => {
    if (days < 1) return 'Less than a day';
    const weeks = Math.floor(days / 7);
    const remainingDays = Math.floor(days % 7);
    if (weeks === 0) return `${remainingDays} day${remainingDays !== 1 ? 's' : ''}`;
    return `${weeks}w ${remainingDays}d`;
  };

  return (
    <View style={styles.container}>
      <Text style={styles.sectionTitle}>PLANT STATUS</Text>

      <View style={styles.infoRow}>
        <View style={styles.infoItem}>
          <Ionicons 
            name={lightOn ? 'sunny' : 'moon'} 
            size={24} 
            color={lightOn ? colors.warning : colors.secondary} 
          />
          <Text style={styles.infoLabel}>Lights</Text>
          <Text style={[styles.infoValue, { color: lightOn ? colors.warning : colors.secondary }]}>
            {lightOn ? 'ON' : 'OFF'}
          </Text>
        </View>

        <View style={styles.infoItem}>
          <Ionicons name="leaf" size={24} color={colors.primary} />
          <Text style={styles.infoLabel}>Age</Text>
          <Text style={styles.infoValue}>
            {plantTimerActive ? formatAge(plantAgeDays) : 'Not tracking'}
          </Text>
        </View>

        <View style={styles.infoItem}>
          <Ionicons 
            name={growStage === 'veg' ? 'leaf' : 'flower'} 
            size={24} 
            color={growStage === 'veg' ? colors.secondary : '#ff6b9d'} 
          />
          <Text style={styles.infoLabel}>Stage</Text>
          <Text style={[styles.infoValue, { color: growStage === 'veg' ? colors.secondary : '#ff6b9d' }]}>
            {growStage.toUpperCase()}
          </Text>
        </View>
      </View>

      <View style={styles.stageButtons}>
        <TouchableOpacity
          style={[styles.stageButton, growStage === 'veg' && styles.stageButtonActive]}
          onPress={() => onStageChange('veg')}
        >
          <Ionicons name="leaf" size={18} color={growStage === 'veg' ? colors.background : colors.secondary} />
          <Text style={[styles.stageButtonText, growStage === 'veg' && styles.stageButtonTextActive]}>
            VEG
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.stageButton, styles.flowerButton, growStage === 'flower' && styles.flowerButtonActive]}
          onPress={() => onStageChange('flower')}
        >
          <Ionicons name="flower" size={18} color={growStage === 'flower' ? colors.background : '#ff6b9d'} />
          <Text style={[styles.stageButtonText, styles.flowerText, growStage === 'flower' && styles.flowerButtonTextActive]}>
            FLOWER
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.timerButtons}>
        {!plantTimerActive ? (
          <TouchableOpacity style={styles.timerButton} onPress={onTimerStart}>
            <Ionicons name="play" size={18} color={colors.background} />
            <Text style={styles.timerButtonText}>START TIMER</Text>
          </TouchableOpacity>
        ) : (
          <>
            <TouchableOpacity style={[styles.timerButton, styles.stopButton]} onPress={onTimerStop}>
              <Ionicons name="stop" size={18} color={colors.text} />
              <Text style={[styles.timerButtonText, styles.stopButtonText]}>STOP</Text>
            </TouchableOpacity>
            <TouchableOpacity style={[styles.timerButton, styles.resetButton]} onPress={onTimerReset}>
              <Ionicons name="refresh" size={18} color={colors.text} />
              <Text style={[styles.timerButtonText, styles.resetButtonText]}>RESET</Text>
            </TouchableOpacity>
          </>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: colors.surface,
    borderRadius: 16,
    padding: 20,
    borderWidth: 1,
    borderColor: colors.border,
    marginVertical: 8,
  },
  sectionTitle: {
    color: colors.textSecondary,
    fontSize: 12,
    fontWeight: '600',
    letterSpacing: 1,
    marginBottom: 16,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 20,
  },
  infoItem: {
    alignItems: 'center',
  },
  infoLabel: {
    color: colors.textMuted,
    fontSize: 11,
    marginTop: 4,
  },
  infoValue: {
    color: colors.text,
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 2,
  },
  stageButtons: {
    flexDirection: 'row',
    gap: 12,
    marginBottom: 16,
  },
  stageButton: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
    borderRadius: 8,
    borderWidth: 2,
    borderColor: colors.secondary,
    gap: 8,
  },
  stageButtonActive: {
    backgroundColor: colors.secondary,
  },
  flowerButton: {
    borderColor: '#ff6b9d',
  },
  flowerButtonActive: {
    backgroundColor: '#ff6b9d',
  },
  stageButtonText: {
    color: colors.secondary,
    fontWeight: 'bold',
    fontSize: 14,
  },
  stageButtonTextActive: {
    color: colors.background,
  },
  flowerText: {
    color: '#ff6b9d',
  },
  flowerButtonTextActive: {
    color: colors.background,
  },
  timerButtons: {
    flexDirection: 'row',
    gap: 12,
  },
  timerButton: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
    borderRadius: 8,
    backgroundColor: colors.primary,
    gap: 8,
  },
  timerButtonText: {
    color: colors.background,
    fontWeight: 'bold',
    fontSize: 13,
  },
  stopButton: {
    backgroundColor: colors.danger,
  },
  stopButtonText: {
    color: colors.text,
  },
  resetButton: {
    backgroundColor: colors.warning,
  },
  resetButtonText: {
    color: colors.background,
  },
});
