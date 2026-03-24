import { Hash, Lock, ChevronLeft } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import type { Channel, Guild } from '../types/discord';

interface Props {
  guild: Guild | null;
  channels: Channel[];
  selectedId: string | null;
  onSelect: (ch: Channel) => void;
}

const TEXT_TYPES = new Set([0, 5]);

function organizeChannels(channels: Channel[]) {
  const categories = channels
    .filter((c) => c.type === 4)
    .sort((a, b) => a.position - b.position);

  const text = channels.filter((c) => TEXT_TYPES.has(c.type));

  const categorized = categories.map((cat) => ({
    ...cat,
    channels: text
      .filter((c) => c.parent_id === cat.id)
      .sort((a, b) => a.position - b.position),
  }));

  const uncategorized = text
    .filter((c) => !c.parent_id)
    .sort((a, b) => a.position - b.position);

  return { categorized, uncategorized };
}

function guildIconUrl(guild: Guild): string | null {
  if (!guild.icon) return null;
  return `https://cdn.discordapp.com/icons/${guild.id}/${guild.icon}.png?size=64`;
}

export default function ChannelSidebar({ guild, channels, selectedId, onSelect }: Props) {
  const navigate = useNavigate();
  const { categorized, uncategorized } = organizeChannels(channels);

  function ChannelItem({ ch }: { ch: Channel }) {
    const isSelected = ch.id === selectedId;
    return (
      <button
        onClick={() => onSelect(ch)}
        className="w-full flex items-center gap-2 px-3 py-1.5 rounded-lg text-sm transition-colors text-left"
        style={{
          background: isSelected ? 'var(--accent-glow)' : 'transparent',
          color: isSelected ? 'var(--accent-text)' : 'var(--text-muted)',
          border: isSelected ? '1px solid rgba(124,58,237,0.2)' : '1px solid transparent',
          cursor: 'pointer',
        }}
        onMouseEnter={(e) => {
          if (!isSelected) {
            e.currentTarget.style.background = 'var(--bg-hover)';
            e.currentTarget.style.color = 'var(--text)';
          }
        }}
        onMouseLeave={(e) => {
          if (!isSelected) {
            e.currentTarget.style.background = 'transparent';
            e.currentTarget.style.color = 'var(--text-muted)';
          }
        }}
      >
        {ch.nsfw ? (
          <Lock size={13} className="shrink-0" style={{ opacity: 0.5 }} />
        ) : (
          <Hash size={13} className="shrink-0" style={{ opacity: 0.6 }} />
        )}
        <span className="truncate">{ch.name}</span>
      </button>
    );
  }

  return (
    <aside
      className="flex flex-col h-full overflow-hidden"
      style={{
        width: '220px',
        minWidth: '220px',
        background: 'var(--bg-surface)',
        borderRight: '1px solid var(--border)',
      }}
    >
      {/* Server header */}
      <div
        className="px-4 py-3 flex items-center gap-3 shrink-0"
        style={{ borderBottom: '1px solid var(--border)' }}
      >
        <button
          onClick={() => navigate('/servers')}
          className="p-1 rounded-lg transition-colors"
          style={{ color: 'var(--text-dim)' }}
          onMouseEnter={(e) => (e.currentTarget.style.color = 'var(--text)')}
          onMouseLeave={(e) => (e.currentTarget.style.color = 'var(--text-dim)')}
        >
          <ChevronLeft size={16} />
        </button>

        {guild && (
          <div className="flex items-center gap-2 min-w-0">
            {guildIconUrl(guild) ? (
              <img
                src={guildIconUrl(guild)!}
                alt=""
                className="w-6 h-6 rounded-md object-cover shrink-0"
              />
            ) : (
              <div
                className="w-6 h-6 rounded-md shrink-0 flex items-center justify-center text-[10px] font-bold"
                style={{ background: 'var(--accent-glow)', color: 'var(--accent-text)' }}
              >
                {guild.name[0].toUpperCase()}
              </div>
            )}
            <span className="text-sm font-semibold truncate" style={{ color: 'var(--text)' }}>
              {guild.name}
            </span>
          </div>
        )}
      </div>

      {/* Channel list */}
      <div className="flex-1 overflow-y-auto py-2 px-2">
        {/* Uncategorized */}
        {uncategorized.map((ch) => (
          <ChannelItem key={ch.id} ch={ch} />
        ))}

        {/* Categorized */}
        {categorized.map((cat) => (
          <div key={cat.id} className="mt-4 mb-1">
            <div
              className="px-3 py-1 text-[10px] font-bold uppercase tracking-widest mb-1"
              style={{ color: 'var(--text-dim)' }}
            >
              {cat.name}
            </div>
            {cat.channels.map((ch) => (
              <ChannelItem key={ch.id} ch={ch} />
            ))}
            {cat.channels.length === 0 && (
              <div className="px-3 py-1 text-xs" style={{ color: 'var(--text-dim)' }}>
                No text channels
              </div>
            )}
          </div>
        ))}

        {channels.length === 0 && (
          <div className="px-3 py-4 text-xs" style={{ color: 'var(--text-dim)' }}>
            No channels available
          </div>
        )}
      </div>
    </aside>
  );
}
