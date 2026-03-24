import { useState } from 'react';
import { Copy, Bookmark, BookmarkCheck, Check, Paperclip, Image } from 'lucide-react';
import type { Message, Attachment } from '../types/discord';

interface Props {
  message: Message;
  saved: boolean;
  onSave: (m: Message) => void;
  onUnsave: (id: string) => void;
}

// ─── Content renderer ─────────────────────────────────────────────────────────

function CodeBlock({ lang, code }: { lang: string; code: string }) {
  const [copied, setCopied] = useState(false);

  function copy() {
    navigator.clipboard.writeText(code).then(() => {
      setCopied(true);
      setTimeout(() => setCopied(false), 1500);
    });
  }

  return (
    <div
      className="rounded-xl overflow-hidden my-2"
      style={{ border: '1px solid var(--border)' }}
    >
      <div
        className="flex items-center justify-between px-4 py-2"
        style={{ background: 'var(--bg-base)', borderBottom: '1px solid var(--border)' }}
      >
        <span className="text-[10px] font-mono font-medium uppercase tracking-widest" style={{ color: 'var(--text-dim)' }}>
          {lang || 'code'}
        </span>
        <button
          onClick={copy}
          className="flex items-center gap-1 text-[10px] px-2 py-1 rounded-md transition-colors"
          style={{
            background: 'var(--bg-elevated)',
            color: copied ? 'var(--green)' : 'var(--text-muted)',
            border: '1px solid var(--border)',
          }}
        >
          {copied ? <Check size={10} /> : <Copy size={10} />}
          {copied ? 'Copied' : 'Copy'}
        </button>
      </div>
      <pre
        className="px-4 py-3 overflow-x-auto text-sm leading-relaxed"
        style={{
          background: 'var(--bg-surface)',
          color: 'var(--text)',
          fontFamily: "'JetBrains Mono', 'Fira Code', 'Consolas', monospace",
          margin: 0,
          whiteSpace: 'pre',
        }}
      >
        <code>{code}</code>
      </pre>
    </div>
  );
}

function renderTextSegment(text: string, key: number) {
  const parts = text.split(/(\b0x[0-9A-Fa-f]{3,}\b)/g);
  return (
    <span key={key}>
      {parts.map((part, i) => {
        if (/^0x[0-9A-Fa-f]{3,}$/i.test(part)) {
          return (
            <span
              key={i}
              className="font-mono font-bold px-1 rounded text-sm"
              style={{ color: 'var(--offset)', background: 'var(--offset-bg)' }}
            >
              {part}
            </span>
          );
        }
        return <span key={i}>{part}</span>;
      })}
    </span>
  );
}

function renderContent(content: string) {
  const parts: React.ReactNode[] = [];
  const codeBlockRe = /```([\w]*)\n?([\s\S]*?)```|`([^`\n]+)`/g;
  let lastIndex = 0;
  let match;
  let idx = 0;

  while ((match = codeBlockRe.exec(content)) !== null) {
    if (match.index > lastIndex) {
      parts.push(renderTextSegment(content.slice(lastIndex, match.index), idx++));
    }
    if (match[2] !== undefined) {
      parts.push(<CodeBlock key={idx++} lang={match[1] ?? ''} code={match[2].trimEnd()} />);
    } else {
      parts.push(
        <code
          key={idx++}
          className="px-1.5 py-0.5 rounded text-sm font-mono"
          style={{ background: 'var(--bg-elevated)', color: 'var(--accent-text)', border: '1px solid var(--border)' }}
        >
          {match[3]}
        </code>,
      );
    }
    lastIndex = match.index + match[0].length;
  }

  if (lastIndex < content.length) {
    parts.push(renderTextSegment(content.slice(lastIndex), idx++));
  }

  return parts;
}

// ─── Attachment renderer ──────────────────────────────────────────────────────

function AttachmentChip({ att }: { att: Attachment }) {
  const isImage = att.content_type?.startsWith('image/');
  const kb = (att.size / 1024).toFixed(1);

  return (
    <a
      href={att.url}
      target="_blank"
      rel="noreferrer"
      className="flex items-center gap-2 px-3 py-2 rounded-xl text-xs transition-colors"
      style={{
        background: 'var(--bg-elevated)',
        border: '1px solid var(--border)',
        color: 'var(--text-muted)',
        textDecoration: 'none',
        display: 'inline-flex',
      }}
      onMouseEnter={(e) => (e.currentTarget.style.borderColor = 'var(--accent)')}
      onMouseLeave={(e) => (e.currentTarget.style.borderColor = 'var(--border)')}
    >
      {isImage ? (
        <Image size={13} style={{ color: 'var(--accent-text)' }} />
      ) : (
        <Paperclip size={13} style={{ color: 'var(--text-dim)' }} />
      )}
      <span className="font-medium" style={{ color: 'var(--text)' }}>{att.filename}</span>
      <span style={{ color: 'var(--text-dim)' }}>{kb} KB</span>
    </a>
  );
}

// ─── Timestamp ────────────────────────────────────────────────────────────────

function formatTime(iso: string): string {
  const d = new Date(iso);
  const now = Date.now();
  const diff = now - d.getTime();

  if (diff < 60_000) return 'just now';
  if (diff < 3_600_000) return `${Math.floor(diff / 60_000)}m ago`;
  if (diff < 86_400_000) return `${Math.floor(diff / 3_600_000)}h ago`;
  return d.toLocaleDateString('en-US', { month: 'short', day: 'numeric' });
}

function avatarUrl(authorId: string, avatarHash: string | null): string {
  if (!avatarHash) {
    return `https://cdn.discordapp.com/embed/avatars/${parseInt(authorId) % 5}.png`;
  }
  return `https://cdn.discordapp.com/avatars/${authorId}/${avatarHash}.png?size=40`;
}

// ─── Message Card ─────────────────────────────────────────────────────────────

export default function MessageCard({ message, saved, onSave, onUnsave }: Props) {
  const [copiedMsg, setCopiedMsg] = useState(false);
  const [hovered, setHovered] = useState(false);

  const { author } = message;
  const displayName = author.global_name ?? author.username;

  function copyMsg() {
    navigator.clipboard.writeText(message.content).then(() => {
      setCopiedMsg(true);
      setTimeout(() => setCopiedMsg(false), 1500);
    });
  }

  function toggleSave() {
    if (saved) onUnsave(message.id);
    else onSave(message);
  }

  return (
    <div
      className="group relative flex gap-3 px-4 py-3 rounded-xl transition-all"
      style={{
        background: hovered ? 'var(--bg-elevated)' : 'transparent',
        border: `1px solid ${hovered ? 'var(--border)' : 'transparent'}`,
      }}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      {/* Avatar */}
      <img
        src={avatarUrl(author.id, author.avatar)}
        alt=""
        className="w-8 h-8 rounded-full shrink-0 mt-0.5"
        style={{ border: '1px solid var(--border)' }}
        onError={(e) => {
          e.currentTarget.src = `https://cdn.discordapp.com/embed/avatars/0.png`;
        }}
      />

      {/* Content */}
      <div className="flex-1 min-w-0">
        {/* Author + timestamp */}
        <div className="flex items-baseline gap-2 mb-1">
          <span className="text-sm font-semibold" style={{ color: 'var(--accent-text)' }}>
            {displayName}
          </span>
          <span className="text-[11px]" style={{ color: 'var(--text-dim)' }}>
            {formatTime(message.timestamp)}
          </span>
          {message.edited_timestamp && (
            <span className="text-[10px]" style={{ color: 'var(--text-dim)' }}>(edited)</span>
          )}
        </div>

        {/* Message body */}
        {message.content && (
          <div className="text-sm leading-relaxed" style={{ color: 'var(--text)', wordBreak: 'break-word' }}>
            {renderContent(message.content)}
          </div>
        )}

        {/* Attachments */}
        {message.attachments.length > 0 && (
          <div className="flex flex-wrap gap-2 mt-2">
            {message.attachments.map((att) => (
              <AttachmentChip key={att.id} att={att} />
            ))}
          </div>
        )}
      </div>

      {/* Action buttons (appear on hover) */}
      {hovered && (
        <div
          className="absolute top-2 right-3 flex items-center gap-1"
        >
          <button
            onClick={toggleSave}
            className="p-1.5 rounded-lg transition-colors"
            style={{
              background: saved ? 'rgba(124,58,237,0.2)' : 'var(--bg-base)',
              color: saved ? 'var(--accent-text)' : 'var(--text-dim)',
              border: `1px solid ${saved ? 'rgba(124,58,237,0.3)' : 'var(--border)'}`,
            }}
            title={saved ? 'Remove from extracts' : 'Save to extracts'}
          >
            {saved ? <BookmarkCheck size={13} /> : <Bookmark size={13} />}
          </button>
          <button
            onClick={copyMsg}
            className="p-1.5 rounded-lg transition-colors"
            style={{
              background: 'var(--bg-base)',
              color: copiedMsg ? 'var(--green)' : 'var(--text-dim)',
              border: '1px solid var(--border)',
            }}
            title="Copy message"
          >
            {copiedMsg ? <Check size={13} /> : <Copy size={13} />}
          </button>
        </div>
      )}
    </div>
  );
}
