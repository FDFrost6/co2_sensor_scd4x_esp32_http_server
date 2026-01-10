# GrowController Mobile App

A React Native (Expo) mobile app for iOS and Android to monitor and control your GrowController ESP32 device.

## Features

- Real-time sensor monitoring (Temperature, Humidity, CO2, VPD)
- VPD status and recommendations
- Growth stage management (Veg/Flower)
- Plant age timer
- Light schedule control
- Device discovery via mDNS
- Dark theme optimized for grow room use

## Prerequisites

- Node.js 18+ 
- npm or yarn
- Expo Go app on your phone (for development)
- GrowController ESP32 device on the same network

## Quick Start

```bash
cd mobile-app
npm install
npm start
```

Then scan the QR code with:
- **iOS**: Camera app
- **Android**: Expo Go app

## Development

### Run on Device

```bash
npm start
```

### Run on iOS Simulator (macOS only)

```bash
npm run ios
```

### Run on Android Emulator

```bash
npm run android
```

## Building for Production

### Setup EAS Build

```bash
npm install -g eas-cli
eas login
eas build:configure
```

### Build for Android

```bash
eas build --platform android
```

### Build for iOS

```bash
eas build --platform ios
```

## Connecting to GrowController

1. Ensure your ESP32 is powered on and connected to WiFi
2. Open the app and go to Settings
3. Enter the device address:
   - Try `growcontroller.local` (mDNS)
   - Or enter the IP address directly (e.g., `192.168.1.100`)
4. Tap Connect

## API Endpoints Used

The app communicates with the ESP32 via HTTP:

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/status` | GET | Current sensor readings |
| `/api/info` | GET | Device information |
| `/data` | GET | Historical data (7 days) |
| `/stage/veg` | GET | Set vegetative stage |
| `/stage/flower` | GET | Set flowering stage |
| `/timer/start` | GET | Start plant timer |
| `/timer/stop` | GET | Stop plant timer |
| `/timer/reset` | GET | Reset plant timer |
| `/light/set?on=X&off=Y` | GET | Set light schedule |

## Project Structure

```
mobile-app/
├── App.tsx                 # Main app entry
├── src/
│   ├── components/         # Reusable UI components
│   │   ├── MetricCard.tsx
│   │   ├── VpdIndicator.tsx
│   │   └── PlantStatus.tsx
│   ├── screens/            # App screens
│   │   ├── DashboardScreen.tsx
│   │   └── SettingsScreen.tsx
│   ├── services/           # API communication
│   │   └── api.ts
│   ├── store/              # State management (Zustand)
│   │   └── deviceStore.ts
│   ├── hooks/              # Custom React hooks
│   │   └── usePolling.ts
│   ├── theme/              # Colors and styles
│   │   └── colors.ts
│   └── types/              # TypeScript types
│       └── index.ts
└── assets/                 # App icons and images
```

## Troubleshooting

### Can't connect to device

1. Verify ESP32 and phone are on the same WiFi network
2. Try using IP address instead of mDNS hostname
3. Check that the ESP32 serial monitor shows the correct IP
4. Ensure no firewall is blocking port 80

### App shows "Not Connected"

1. Go to Settings
2. Enter device address and tap Connect
3. If discovery fails, enter IP manually

### Data not updating

1. Pull down to refresh
2. Check device is still reachable
3. Verify ESP32 is running (check LED indicators)

## License

MIT License
