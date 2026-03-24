import type { FilterType, Message } from '../types/discord';
import { matchesFilter } from '../types/discord';

interface FilterBarProps {
  active: FilterType;
  onChange: (f: FilterType) => void;
  messages: Message[];
}

const FILTERS: { key: FilterType; label: string }[] = [
  { key: 'all',     label: 'All'     },
  { key: 'code',    label: 'Code'    },
  { key: 'offsets', label: 'Offsets' },
  { key: 'files',   label: 'Files'   },
  { key: 'links',   label: 'Links'   },
];

export default function FilterBar({ active, onChange, messages }: FilterBarProps) {
  function count(f: FilterType): number {
    if (f === 'all') return messages.length;
    return messages.filter((m) => matchesFilter(m, f)).length;
  }

  return (
    <div className="flex items-center gap-1.5 flex-wrap">
      {FILTERS.map(({ key, label }) => {
        const n = count(key);
        const isActive = active === key;
        return (
          <button
            key={key}
            onClick={() => onChange(key)}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all"
            style={{
              background: isActive ? 'var(--accent)' : 'var(--bg-elevated)',
              color: isActive ? '#fff' : 'var(--text-muted)',
              border: `1px solid ${isActive ? 'var(--accent)' : 'var(--border)'}`,
              cursor: 'pointer',
            }}
          >
            {label}
            {n > 0 && (
              <span
                className="rounded-md px-1.5 py-0.5 text-[10px] font-bold"
                style={{
                  background: isActive ? 'rgba(255,255,255,0.2)' : 'var(--bg-base)',
                  color: isActive ? '#fff' : 'var(--text-dim)',
                }}
              >
                {n}
              </span>
            )}
          </button>
        );
      })}
    </div>
  );
}
