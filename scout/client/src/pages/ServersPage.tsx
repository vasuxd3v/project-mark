import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Crosshair, LogOut, Search, ServerCrash } from 'lucide-react';
import { getGuilds, getMe } from '../api/client';
import { useAuth } from '../App';
import type { CurrentUser, Guild } from '../types/discord';

function avatarUrl(user: CurrentUser): string | null {
  if (!user.avatar) return null;
  return `https://cdn.discordapp.com/avatars/${user.id}/${user.avatar}.png?size=64`;
}

function guildIconUrl(guild: Guild): string | null {
  if (!guild.icon) return null;
  return `https://cdn.discordapp.com/icons/${guild.id}/${guild.icon}.png?size=64`;
}

function GuildInitial({ name }: { name: string }) {
  const initials = name
    .split(/[\s\-_]+/)
    .slice(0, 2)
    .map((w) => w[0]?.toUpperCase() ?? '')
    .join('');

  return (
    <div
      className="w-full h-full flex items-center justify-center text-base font-bold select-none"
      style={{ color: 'var(--accent-text)', background: 'var(--accent-glow)' }}
    >
      {initials}
    </div>
  );
}

export default function ServersPage() {
  const { user: ctxUser, token, logout } = useAuth();
  const navigate = useNavigate();

  const [user, setUser] = useState<CurrentUser | null>(ctxUser);
  const [guilds, setGuilds] = useState<Guild[]>([]);
  const [loading, setLoading] = useState(true);
  const [search, setSearch] = useState('');
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    async function load() {
      try {
        const [u, g] = await Promise.all([getMe(), getGuilds()]);
        setUser(u);
        setGuilds(g.sort((a, b) => a.name.localeCompare(b.name)));
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to load');
      } finally {
        setLoading(false);
      }
    }
    load();
  }, [token]);

  const filtered = guilds.filter((g) =>
    g.name.toLowerCase().includes(search.toLowerCase()),
  );

  function handleLogout() {
    logout();
    navigate('/', { replace: true });
  }

  return (
    <div
      className="min-h-screen flex flex-col"
      style={{ background: 'var(--bg-base)' }}
    >
      {/* Header */}
      <header
        className="flex items-center justify-between px-6 py-4 sticky top-0 z-10"
        style={{
          background: 'var(--bg-surface)',
          borderBottom: '1px solid var(--border)',
        }}
      >
        <div className="flex items-center gap-2">
          <Crosshair size={18} style={{ color: 'var(--accent-text)' }} strokeWidth={1.5} />
          <span className="font-semibold text-sm tracking-wide" style={{ color: 'var(--text)' }}>
            scout
          </span>
        </div>

        <div className="flex items-center gap-4">
          {user && (
            <div className="flex items-center gap-2">
              {avatarUrl(user) ? (
                <img
                  src={avatarUrl(user)!}
                  alt=""
                  className="w-7 h-7 rounded-full"
                  style={{ border: '1px solid var(--border)' }}
                />
              ) : (
                <div
                  className="w-7 h-7 rounded-full flex items-center justify-center text-xs font-bold"
                  style={{ background: 'var(--accent-glow)', color: 'var(--accent-text)' }}
                >
                  {(user.global_name ?? user.username)[0].toUpperCase()}
                </div>
              )}
              <span className="text-sm font-medium" style={{ color: 'var(--text-muted)' }}>
                {user.global_name ?? user.username}
              </span>
            </div>
          )}
          <button
            onClick={handleLogout}
            className="flex items-center gap-1.5 text-xs px-3 py-1.5 rounded-lg transition-colors"
            style={{
              background: 'var(--bg-elevated)',
              border: '1px solid var(--border)',
              color: 'var(--text-muted)',
            }}
          >
            <LogOut size={13} />
            Logout
          </button>
        </div>
      </header>

      {/* Main */}
      <main className="flex-1 max-w-5xl w-full mx-auto px-6 py-8">
        {/* Title + search */}
        <div className="flex items-center justify-between mb-6">
          <div>
            <h2 className="text-lg font-semibold" style={{ color: 'var(--text)' }}>
              Your Servers
            </h2>
            <p className="text-sm mt-0.5" style={{ color: 'var(--text-muted)' }}>
              {guilds.length} server{guilds.length !== 1 ? 's' : ''} — select one to explore channels
            </p>
          </div>

          <div className="relative">
            <Search
              size={14}
              className="absolute left-3 top-1/2 -translate-y-1/2"
              style={{ color: 'var(--text-dim)' }}
            />
            <input
              type="text"
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              placeholder="Filter servers…"
              className="pl-9 pr-4 py-2 rounded-xl text-sm outline-none transition-colors"
              style={{
                background: 'var(--bg-elevated)',
                border: '1px solid var(--border)',
                color: 'var(--text)',
                width: '220px',
              }}
            />
          </div>
        </div>

        {/* Loading */}
        {loading && (
          <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3">
            {Array.from({ length: 8 }).map((_, i) => (
              <div
                key={i}
                className="rounded-2xl p-4 flex items-center gap-3"
                style={{
                  background: 'var(--bg-surface)',
                  border: '1px solid var(--border-dim)',
                  opacity: 1 - i * 0.1,
                }}
              >
                <div
                  className="w-10 h-10 rounded-xl shrink-0"
                  style={{ background: 'var(--bg-elevated)' }}
                />
                <div className="flex-1 min-w-0">
                  <div
                    className="h-3 rounded-md mb-2"
                    style={{ background: 'var(--bg-elevated)', width: '70%' }}
                  />
                  <div
                    className="h-2 rounded-md"
                    style={{ background: 'var(--bg-elevated)', width: '40%' }}
                  />
                </div>
              </div>
            ))}
          </div>
        )}

        {/* Error */}
        {error && !loading && (
          <div
            className="flex flex-col items-center justify-center py-20 gap-3"
          >
            <ServerCrash size={32} style={{ color: 'var(--text-dim)' }} strokeWidth={1} />
            <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
              {error}
            </p>
          </div>
        )}

        {/* Guild grid */}
        {!loading && !error && (
          <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3">
            {filtered.map((guild) => (
              <button
                key={guild.id}
                onClick={() => navigate(`/servers/${guild.id}`)}
                className="rounded-2xl p-4 flex items-center gap-3 text-left transition-all group"
                style={{
                  background: 'var(--bg-surface)',
                  border: '1px solid var(--border)',
                  cursor: 'pointer',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.borderColor = 'var(--accent)';
                  e.currentTarget.style.background = 'var(--bg-elevated)';
                  e.currentTarget.style.boxShadow = '0 0 16px var(--accent-glow)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.borderColor = 'var(--border)';
                  e.currentTarget.style.background = 'var(--bg-surface)';
                  e.currentTarget.style.boxShadow = 'none';
                }}
              >
                <div
                  className="w-10 h-10 rounded-xl shrink-0 overflow-hidden"
                  style={{ border: '1px solid var(--border)' }}
                >
                  {guildIconUrl(guild) ? (
                    <img
                      src={guildIconUrl(guild)!}
                      alt=""
                      className="w-full h-full object-cover"
                    />
                  ) : (
                    <GuildInitial name={guild.name} />
                  )}
                </div>
                <div className="flex-1 min-w-0">
                  <p
                    className="text-sm font-medium truncate"
                    style={{ color: 'var(--text)' }}
                  >
                    {guild.name}
                  </p>
                  {guild.owner && (
                    <p className="text-xs mt-0.5" style={{ color: 'var(--accent-text)' }}>
                      Owner
                    </p>
                  )}
                </div>
              </button>
            ))}

            {filtered.length === 0 && (
              <div
                className="col-span-4 text-center py-16 text-sm"
                style={{ color: 'var(--text-dim)' }}
              >
                No servers match "{search}"
              </div>
            )}
          </div>
        )}
      </main>
    </div>
  );
}
