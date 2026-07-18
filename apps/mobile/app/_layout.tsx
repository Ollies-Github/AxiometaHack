import { Stack } from 'expo-router';
import { HatProvider } from '../src/ws/HatContext';

export default function RootLayout() {
  return (
    <HatProvider>
      <Stack screenOptions={{ headerShown: false }} />
    </HatProvider>
  );
}
