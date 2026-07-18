import { createContext, useContext, type ReactNode } from 'react';
import { useHatWs } from './useHatWs';

type HatContextValue = ReturnType<typeof useHatWs>;

const HatContext = createContext<HatContextValue | null>(null);

// Holds the single shared hat connection for the whole app, so Home and the
// control panel talk over one WebSocket and see the same status/apps/log.
export function HatProvider({ children }: { children: ReactNode }) {
  const hat = useHatWs();
  return <HatContext.Provider value={hat}>{children}</HatContext.Provider>;
}

export function useHat(): HatContextValue {
  const v = useContext(HatContext);
  if (!v) throw new Error('useHat must be used inside a <HatProvider>');
  return v;
}
