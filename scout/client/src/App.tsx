import { createContext, useContext, useEffect, useState } from 'react';
import { BrowserRouter, Navigate, Route, Routes } from 'react-router-dom';
import { setApiToken } from './api/client';
import TokenPage from './pages/TokenPage';
import ServersPage from './pages/ServersPage';
import ChannelsPage from './pages/ChannelsPage';
import type { CurrentUser } from './types/discord';

// ─── Auth Context ─────────────────────────────────────────────────────────────

interface AuthCtx {
  user: CurrentUser | null;
  token: string | null;
  login: (token: string, user: CurrentUser) => void;
  logout: () => void;
}

const AuthContext = createContext<AuthCtx>({
  user: null,
  token: null,
  login: () => {},
  logout: () => {},
});

export function useAuth() {
  return useContext(AuthContext);
}

// ─── App ──────────────────────────────────────────────────────────────────────

export default function App() {
  const [user, setUser] = useState<CurrentUser | null>(null);
  const [token, setTokenState] = useState<string | null>(null);

  // Restore token from localStorage on mount
  useEffect(() => {
    const saved = localStorage.getItem('scout_token');
    if (saved) {
      setApiToken(saved);
      setTokenState(saved);
    }
  }, []);

  const login = (t: string, u: CurrentUser) => {
    localStorage.setItem('scout_token', t);
    setApiToken(t);
    setTokenState(t);
    setUser(u);
  };

  const logout = () => {
    localStorage.removeItem('scout_token');
    setApiToken(null);
    setTokenState(null);
    setUser(null);
  };

  return (
    <AuthContext.Provider value={{ user, token, login, logout }}>
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<TokenPage />} />
          <Route
            path="/servers"
            element={token ? <ServersPage /> : <Navigate to="/" replace />}
          />
          <Route
            path="/servers/:guildId"
            element={token ? <ChannelsPage /> : <Navigate to="/" replace />}
          />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </BrowserRouter>
    </AuthContext.Provider>
  );
}
