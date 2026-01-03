/*
 * VPD (Vapor Pressure Deficit) Calculator for Cannabis Grow Monitoring
 * 
 * Calculates VPD using standard atmospheric formulas and classifies
 * readings based on cannabis-specific target ranges for vegetative
 * and flowering growth stages.
 * 
 * VPD Formula:
 *   es = 0.6108 * exp((17.27 * T) / (T + 237.3))  [saturation vapor pressure in kPa]
 *   ea = es * RH / 100.0                           [actual vapor pressure in kPa]
 *   VPD = es - ea                                  [vapor pressure deficit in kPa]
 * 
 * Cannabis Target Ranges:
 *   Vegetative: 0.8 - 1.2 kPa
 *   Flowering:  1.2 - 1.6 kPa
 * 
 * Written for ESP32 cannabis grow monitoring
 * License: MIT
 */

#ifndef VPD_H
#define VPD_H

#include <Arduino.h>
#include <math.h>

// ========================================
// Enums and Structs
// ========================================

enum class GrowStage {
    VEG,      // Vegetative stage
    FLOWER    // Flowering stage
};

enum class VpdStatus {
    TOO_LOW,  // VPD below optimal range
    OPTIMAL,  // VPD within target range
    TOO_HIGH  // VPD above optimal range
};

struct VpdRange {
    float min_kPa;  // Lower bound of optimal VPD range
    float max_kPa;  // Upper bound of optimal VPD range
};

// ========================================
// Global Configuration
// ========================================

// Set your current grow stage here:
// GrowStage::VEG for vegetative, GrowStage::FLOWER for flowering
extern GrowStage currentGrowStage;

// ========================================
// VPD Calculation Functions
// ========================================

/**
 * Calculate saturation vapor pressure (es) in kPa
 * Uses the Magnus-Tetens approximation
 * 
 * @param tempC Temperature in degrees Celsius
 * @return Saturation vapor pressure in kPa
 */
inline float saturationVaporPressure(float tempC) {
    return 0.6108 * exp((17.27 * tempC) / (tempC + 237.3));
}

/**
 * Calculate actual vapor pressure (ea) in kPa
 * 
 * @param tempC Temperature in degrees Celsius
 * @param rhPercent Relative humidity as a percentage (0-100)
 * @return Actual vapor pressure in kPa
 */
inline float actualVaporPressure(float tempC, float rhPercent) {
    float es = saturationVaporPressure(tempC);
    return es * rhPercent / 100.0;
}

/**
 * Calculate Vapor Pressure Deficit (VPD) in kPa
 * 
 * @param tempC Temperature in degrees Celsius
 * @param rhPercent Relative humidity as a percentage (0-100)
 * @return VPD in kPa
 */
inline float computeVPD(float tempC, float rhPercent) {
    float es = saturationVaporPressure(tempC);
    float ea = actualVaporPressure(tempC, rhPercent);
    return es - ea;
}

// ========================================
// Cannabis-Specific Range Functions
// ========================================

/**
 * Get the optimal VPD range for a given grow stage and plant age
 * 
 * Cannabis-specific ranges based on grower best practices:
 *   Seedling (0-14 days):    0.4 - 0.8 kPa (high humidity for root development)
 *   Early Veg (15-28 days):  0.8 - 1.0 kPa (transitioning)
 *   Late Veg (29+ days):     1.0 - 1.2 kPa (building structure)
 *   Early Flower (0-21 days):1.0 - 1.3 kPa (transition period)
 *   Mid Flower (22-49 days): 1.2 - 1.5 kPa (bulk building)
 *   Late Flower (50+ days):  1.3 - 1.6 kPa (preventing mold)
 * 
 * @param stage The current grow stage
 * @param plantAgeDays Age of the plant in days (0 if not tracking)
 * @return VpdRange struct with min and max kPa values
 */
inline VpdRange getVpdRangeForStage(GrowStage stage, float plantAgeDays = -1) {
    VpdRange range;
    
    // If plant age is tracked, use age-based ranges for precision
    if (plantAgeDays >= 0) {
        if (stage == GrowStage::VEG) {
            // Vegetative stage age-based ranges
            if (plantAgeDays <= 14) {
                // Seedling stage
                range.min_kPa = 0.4;
                range.max_kPa = 0.8;
            } else if (plantAgeDays <= 28) {
                // Early vegetative
                range.min_kPa = 0.8;
                range.max_kPa = 1.0;
            } else {
                // Late vegetative
                range.min_kPa = 1.0;
                range.max_kPa = 1.2;
            }
        } else {
            // Flowering stage age-based ranges
            // Note: plantAgeDays counts from germination, not flip to flower
            // This assumes flower started after ~30 days of veg
            float flowerDays = plantAgeDays - 30;  // Approximate days since flower
            
            if (flowerDays <= 21) {
                // Early flower / stretch phase
                range.min_kPa = 1.0;
                range.max_kPa = 1.3;
            } else if (flowerDays <= 49) {
                // Mid flower / bulk building
                range.min_kPa = 1.2;
                range.max_kPa = 1.5;
            } else {
                // Late flower / ripening
                range.min_kPa = 1.3;
                range.max_kPa = 1.6;
            }
        }
    } else {
        // Fallback: Use standard stage-based ranges if age not tracked
        switch (stage) {
            case GrowStage::VEG:
                range.min_kPa = 0.8;
                range.max_kPa = 1.2;
                break;
            case GrowStage::FLOWER:
                range.min_kPa = 1.2;
                range.max_kPa = 1.6;
                break;
            default:
                // Default to veg range
                range.min_kPa = 0.8;
                range.max_kPa = 1.2;
                break;
        }
    }
    
    return range;
}

/**
 * Classify the current VPD reading against optimal range for stage and age
 * 
 * @param vpd Current VPD value in kPa
 * @param stage Current grow stage
 * @param plantAgeDays Age of plant in days (optional, -1 to use stage-only)
 * @return VpdStatus indicating if VPD is too low, optimal, or too high
 */
inline VpdStatus classifyVpd(float vpd, GrowStage stage, float plantAgeDays = -1) {
    VpdRange range = getVpdRangeForStage(stage, plantAgeDays);
    
    if (vpd < range.min_kPa) {
        return VpdStatus::TOO_LOW;
    } else if (vpd > range.max_kPa) {
        return VpdStatus::TOO_HIGH;
    } else {
        return VpdStatus::OPTIMAL;
    }
}

// ========================================
// String Conversion Functions
// ========================================

/**
 * Convert VpdStatus enum to human-readable string
 * 
 * @param status VpdStatus enum value
 * @return String representation ("too_low", "optimal", "too_high")
 */
inline const char* vpdStatusToString(VpdStatus status) {
    switch (status) {
        case VpdStatus::TOO_LOW:
            return "too_low";
        case VpdStatus::OPTIMAL:
            return "optimal";
        case VpdStatus::TOO_HIGH:
            return "too_high";
        default:
            return "unknown";
    }
}

/**
 * Convert GrowStage enum to human-readable string
 * 
 * @param stage GrowStage enum value
 * @return String representation ("veg", "flower")
 */
inline const char* growStageToString(GrowStage stage) {
    switch (stage) {
        case GrowStage::VEG:
            return "veg";
        case GrowStage::FLOWER:
            return "flower";
        default:
            return "unknown";
    }
}

/**
 * Convert VpdStatus enum to numeric value for Prometheus
 * Useful for alerting/graphing in Prometheus/Grafana
 * 
 * @param status VpdStatus enum value
 * @return Numeric value (-1 = too_low, 0 = optimal, 1 = too_high)
 */
inline int vpdStatusToNumeric(VpdStatus status) {
    switch (status) {
        case VpdStatus::TOO_LOW:
            return -1;
        case VpdStatus::OPTIMAL:
            return 0;
        case VpdStatus::TOO_HIGH:
            return 1;
        default:
            return -999;  // Error value
    }
}

#endif // VPD_H
