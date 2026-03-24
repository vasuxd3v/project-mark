import { BookmarkCheck, Copy, Trash2, X, Check, FileText } from 'lucide-react';
import { useState } from 'react';
import type { Message } from '../types/discord';

interface Props {
  messages: Message[];
  onRemove: (id: string) => void;
  onClearAll: () => void;
  onClose: () => void;
}

export default function ExtractPanel({ messages, onRemove, onClearAll, onClose }: Props) {
  const [copiedAll, setCopiedAll] = useState(false);

  function copyAll() {
    const text = messages
      .map((m) => {
        const name = m.author.global_name ?? m.author.username;
        const ts = new Date(m.timestamp).toLocaleString();
        const attachments = m.attachments.map((a) => `[attachment: ${a.filename}]`).join(' ');
        return `[${ts}] ${name}:\n${m.content}${attachments ? '\n' + attachments : ''}`;
      })
      .join('\n\n---\n\n');

    navigator.clipboard.writeText(text).then(() => {
      setCopiedAll(true);
      setTimeout(() => setCopiedAll(false), 2000);
    });
  }

  return (
    <div
      className="slide-in-right flex flex-col h-full"
      style={{
        width: '300px',
        minWidth: '300px',
        background: 'var(--bg-surface)',
        borderLeft: '1px solid var(--border)',
      }}
    >
      {/* Header */}
      <div
        className="flex items-center justify-between px-4 py-3 shrink-0"
        style={{ borderBottom: '1px solid var(--border)' }}
      >
        <div className="flex items-center gap-2">
          <BookmarkCheck size={14} style={{ color: 'var(--accent-text)' }} />
          <span className="text-sm font-semibold" style={{ color: 'var(--text)' }}>
            Extracts
          </span>
          <span
            className="text-xs px-1.5 py-0.5 rounded-md font-mono"
            style={{
              background: 'var(--accent-glow)',
              color: 'var(--accent-text)',
              border: '1px solid rgba(124,58,237,0.2)',
            }}
          >
            {messages.length}
          </span>
        </div>

        <div className="flex items-center gap-1">
          {messages.length > 0 && (
            <>
              <button
                onClick={copyAll}
                className="flex items-center gap-1 text-xs px-2 py-1.5 rounded-lg transition-colors"
                style={{
                  background: 'var(--bg-elevated)',
                  color: copiedAll ? 'var(--green)' : 'var(--text-muted)',
                  border: '1px solid var(--border)',
                }}
                title="Copy all extracts"
              >
                {copiedAll ? <Check size={11} /> : <Copy size={11} />}
                {copiedAll ? 'Copied' : 'Copy all'}
              </button>
              <button
                onClick={onClearAll}
                className="p-1.5 rounded-lg transition-colors"
                style={{
                  background: 'var(--bg-elevated)',
                  color: 'var(--text-dim)',
                  border: '1px solid var(--border)',
                }}
                title="Clear all"
              >
                <Trash2 size={13} />
              </button>
            </>
          )}
          <button
            onClick={onClose}
            className="p-1.5 rounded-lg transition-colors"
            style={{
              background: 'var(--bg-elevated)',
              color: 'var(--text-dim)',
              border: '1px solid var(--border)',
            }}
          >
            <X size={13} />
          </button>
        </div>
      </div>

      {/* Empty state */}
      {messages.length === 0 && (
        <div className="flex-1 flex flex-col items-center justify-center gap-3 px-6 text-center">
          <FileText size={28} style={{ color: 'var(--text-dim)' }} strokeWidth={1} />
          <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
            No extracts yet
          </p>
          <p className="text-xs" style={{ color: 'var(--text-dim)' }}>
            Hover a message and click the bookmark icon to save it here.
          </p>
        </div>
      )}

      {/* Extract list */}
      <div className="flex-1 overflow-y-auto py-2 px-2 flex flex-col gap-2">
        {messages.map((msg) => {
          const name = msg.author.global_name ?? msg.author.username;
          return (
            <div
              key={msg.id}
              className="rounded-xl p-3 relative group"
              style={{
                background: 'var(--bg-elevated)',
                border: '1px solid var(--border)',
              }}
            >
              <div className="flex items-baseline justify-between gap-2 mb-1">
                <span className="text-xs font-semibold" style={{ color: 'var(--accent-text)' }}>
                  {name}
                </span>
                <button
                  onClick={() => onRemove(msg.id)}
                  className="opacity-0 group-hover:opacity-100 p-0.5 rounded transition-opacity"
                  style={{ color: 'var(--text-dim)' }}
                >
                  <X size={11} />
                </button>
              </div>
              <p
                className="text-xs leading-relaxed line-clamp-4"
                style={{
                  color: 'var(--text-muted)',
                  display: '-webkit-box',
                  WebkitLineClamp: 4,
                  WebkitBoxOrient: 'vertical',
                  overflow: 'hidden',
                }}
              >
                {msg.content || (msg.attachments.length > 0 ? `[${msg.attachments.length} attachment(s)]` : '[no content]')}
              </p>
              {msg.attachments.length > 0 && (
                <div className="mt-1.5 flex items-center gap-1 text-[10px]" style={{ color: 'var(--text-dim)' }}>
                  <FileText size={9} />
                  {msg.attachments.map((a) => a.filename).join(', ')}
                </div>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}
