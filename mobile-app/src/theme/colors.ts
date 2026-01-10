export const colors = {
  background: '#0d1117',
  surface: '#161b22',
  surfaceLight: '#21262d',
  border: '#30363d',
  
  primary: '#00ff66',
  primaryDark: '#00cc52',
  secondary: '#00c3ff',
  warning: '#ffcc00',
  danger: '#ff004c',
  
  text: '#ffffff',
  textSecondary: '#8b949e',
  textMuted: '#6e7681',
  
  vpdOptimal: '#00ff66',
  vpdTooLow: '#00c3ff',
  vpdTooHigh: '#ff004c',
  
  temperature: '#ff6b6b',
  humidity: '#4ecdc4',
  co2: '#00c3ff',
  vpd: '#ffcc00',
};

export const shadows = {
  glow: (color: string) => ({
    shadowColor: color,
    shadowOffset: { width: 0, height: 0 },
    shadowOpacity: 0.5,
    shadowRadius: 10,
    elevation: 5,
  }),
};
