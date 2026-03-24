import { useCallback, useEffect, useRef, useState } from 'react';
import { useParams } from 'react-router-dom';
import { Hash, Search, BookmarkCheck, Loader2, ChevronUp, X, AlertCircle } from 'lucide-react';
import { getChannels, getGuilds, getMessages, searchMessages } from '../api/client';
import ChannelSidebar from '../components/ChannelSidebar';
import FilterBar from '../components/FilterBar';
import MessageCard from '../components/MessageCard';
import ExtractPanel from '../components/ExtractPanel';
import type { Channel, FilterType, Guild, Message } from '../types/discord';
import { matchesFilter } from '../types/discord';

const EXTRACT_KEY = 'scout_extracts';

function loadSaved(): Message[] {
  try {
    return JSON.parse(localStorage.getItem(EXTRACT_KEY) ?? '[]') as Message[];
  } catch {
    return [];
  }
}

function persistSaved(msgs: Message[]): void {
  localStorage.setItem(EXTRACT_KEY, JSON.stringify(msgs));
}

export default function ChannelsPage() {
  const { guildId } = useParams<{ guildId: string }>();

  const [guild, setGuild] = useState<Guild | null>(null);
  const [channels, setChannels] = useState<Channel[]>([]);
  const [selectedChannel, setSelectedChannel] = useState<Channel | null>(null);

  const [messages, setMessages] = useState<Message[]>([]);
  const [loadingMessages, setLoadingMessages] = useState(false);
  const [loadingMore, setLoadingMore] = useState(false);
  const [hasMore, setHasMore] = useState(true);
  const [msgError, setMsgError] = useState<string | null>(null);

  const [filter, setFilter] = useState<FilterType>('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [searching, setSearching] = useState(false);
  const [searchResults, setSearchResults] = useState<Message[] | null>(null);

  const [extracts, setExtracts] = useState<Message[]>(loadSaved);
  const [showExtracts, setShowExtracts] = useState(false);

  const messagesEndRef = useRef<HTMLDivElement>(null);
  const searchTimeout = useRef<ReturnType<typeof setTimeout> | undefined>(undefined);

  // Load guild + channels
  useEffect(() => {
    if (!guildId) return;
    Promise.all([getGuilds(), getChannels(guildId)])
      .then(([guilds, chs]) => {
        setGuild(guilds.find((g) => g.id === guildId) ?? null);
        setChannels(chs);
      })
      .catch(console.error);
  }, [guildId]);

  // Load messages when channel changes
  const loadMessages = useCallback(async (ch: Channel) => {
    setMessages([]);
    setHasMore(true);
    setMsgError(null);
    setSearchResults(null);
    setSearchQuery('');
    setFilter('all');
    setLoadingMessages(true);
    try {
      const msgs = await getMessages(ch.id);
      setMessages(msgs);
      setHasMore(msgs.length === 50);
    } catch (err) {
      setMsgError(err instanceof Error ? err.message : 'Failed to load messages');
    } finally {
      setLoadingMessages(false);
    }
  }, []);

  function handleChannelSelect(ch: Channel) {
    setSelectedChannel(ch);
    loadMessages(ch);
  }

  // Load more (older) messages
  async function loadMore() {
    if (!selectedChannel || loadingMore || !hasMore) return;
    const oldest = messages[messages.length - 1]?.id;
    if (!oldest) return;
    setLoadingMore(true);
    try {
      const older = await getMessages(selectedChannel.id, oldest);
      setMessages((prev) => [...prev, ...older]);
      setHasMore(older.length === 50);
    } catch (err) {
      console.error(err);
    } finally {
      setLoadingMore(false);
    }
  }

  // Search (debounced)
  useEffect(() => {
    if (!searchQuery.trim() || !selectedChannel || !guildId) {
      setSearchResults(null);
      return;
    }
    clearTimeout(searchTimeout.current);
    searchTimeout.current = setTimeout(async () => {
      setSearching(true);
      try {
        const result = await searchMessages(guildId, selectedChannel.id, searchQuery.trim());
        setSearchResults(result.messages.map((arr) => arr[0]).filter(Boolean));
      } catch {
        setSearchResults([]);
      } finally {
        setSearching(false);
      }
    }, 600);
    return () => clearTimeout(searchTimeout.current);
  }, [searchQuery, selectedChannel, guildId]);

  // Extract management
  function saveExtract(msg: Message) {
    setExtracts((prev) => {
      if (prev.some((m) => m.id === msg.id)) return prev;
      const updated = [msg, ...prev];
      persistSaved(updated);
      return updated;
    });
    setShowExtracts(true);
  }

  function removeExtract(id: string) {
    setExtracts((prev) => {
      const updated = prev.filter((m) => m.id !== id);
      persistSaved(updated);
      return updated;
    });
  }

  function clearExtracts() {
    setExtracts([]);
    persistSaved([]);
  }

  // Displayed messages
  const displayMessages = searchResults ?? messages;
  const filteredMessages = displayMessages.filter((m) => matchesFilter(m, filter));

  const savedIds = new Set(extracts.map((m) => m.id));

  return (
    <div className="flex h-screen overflow-hidden" style={{ background: 'var(--bg-base)' }}>
      {/* Sidebar */}
      <ChannelSidebar
        guild={guild}
        channels={channels}
        selectedId={selectedChannel?.id ?? null}
        onSelect={handleChannelSelect}
      />

      {/* Main panel */}
      <div className="flex flex-col flex-1 min-w-0 overflow-hidden">
        {/* Top bar */}
        <div
          className="flex items-center justify-between gap-3 px-5 py-3 shrink-0"
          style={{
            background: 'var(--bg-surface)',
            borderBottom: '1px solid var(--border)',
          }}
        >
          <div className="flex items-center gap-2 min-w-0">
            <Hash size={15} style={{ color: 'var(--text-dim)', flexShrink: 0 }} />
            <span className="text-sm font-semibold truncate" style={{ color: 'var(--text)' }}>
              {selectedChannel?.name ?? 'Select a channel'}
            </span>
            {selectedChannel?.topic && (
              <>
                <span style={{ color: 'var(--border)', fontSize: '1.2em' }}>│</span>
                <span className="text-xs truncate" style={{ color: 'var(--text-dim)' }}>
                  {selectedChannel.topic}
                </span>
              </>
            )}
          </div>

          <div className="flex items-center gap-2 shrink-0">
            {/* Search */}
            {selectedChannel && (
              <div className="relative">
                <Search
                  size={13}
                  className="absolute left-3 top-1/2 -translate-y-1/2"
                  style={{ color: 'var(--text-dim)' }}
                />
                <input
                  type="text"
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  placeholder="Search channel…"
                  className="pl-8 pr-8 py-1.5 rounded-lg text-xs outline-none transition-colors"
                  style={{
                    background: 'var(--bg-elevated)',
                    border: '1px solid var(--border)',
                    color: 'var(--text)',
                    width: '200px',
                  }}
                />
                {searching && (
                  <Loader2
                    size={11}
                    className="absolute right-2.5 top-1/2 -translate-y-1/2 spin"
                    style={{ color: 'var(--text-dim)' }}
                  />
                )}
                {searchQuery && !searching && (
                  <button
                    onClick={() => setSearchQuery('')}
                    className="absolute right-2.5 top-1/2 -translate-y-1/2"
                    style={{ color: 'var(--text-dim)' }}
                  >
                    <X size={11} />
                  </button>
                )}
              </div>
            )}

            {/* Extract toggle */}
            <button
              onClick={() => setShowExtracts((s) => !s)}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all"
              style={{
                background: showExtracts ? 'var(--accent-glow)' : 'var(--bg-elevated)',
                color: showExtracts ? 'var(--accent-text)' : 'var(--text-muted)',
                border: `1px solid ${showExtracts ? 'rgba(124,58,237,0.3)' : 'var(--border)'}`,
              }}
            >
              <BookmarkCheck size={13} />
              Extracts
              {extracts.length > 0 && (
                <span
                  className="text-[10px] font-bold px-1.5 rounded-md"
                  style={{ background: 'var(--accent)', color: '#fff' }}
                >
                  {extracts.length}
                </span>
              )}
            </button>
          </div>
        </div>

        {/* Filter bar */}
        {selectedChannel && messages.length > 0 && !searchResults && (
          <div
            className="px-5 py-2.5 shrink-0"
            style={{ borderBottom: '1px solid var(--border-dim)' }}
          >
            <FilterBar active={filter} onChange={setFilter} messages={messages} />
          </div>
        )}

        {/* Search results banner */}
        {searchResults !== null && (
          <div
            className="px-5 py-2 flex items-center justify-between text-xs shrink-0"
            style={{
              background: 'var(--accent-glow)',
              borderBottom: '1px solid rgba(124,58,237,0.2)',
              color: 'var(--accent-text)',
            }}
          >
            <span>
              {searchResults.length} result{searchResults.length !== 1 ? 's' : ''} for "
              {searchQuery}"
            </span>
            <button
              onClick={() => {
                setSearchQuery('');
                setSearchResults(null);
              }}
              className="underline underline-offset-2"
            >
              Clear search
            </button>
          </div>
        )}

        {/* Messages */}
        <div className="flex-1 overflow-y-auto px-3 py-3">
          {/* Empty / no channel */}
          {!selectedChannel && (
            <div className="h-full flex flex-col items-center justify-center gap-3">
              <Hash size={36} style={{ color: 'var(--text-dim)' }} strokeWidth={1} />
              <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
                Select a channel from the sidebar
              </p>
            </div>
          )}

          {/* Loading */}
          {loadingMessages && (
            <div className="flex flex-col items-center justify-center h-64 gap-3">
              <Loader2 size={22} className="spin" style={{ color: 'var(--accent-text)' }} />
              <span className="text-sm" style={{ color: 'var(--text-muted)' }}>
                Loading messages…
              </span>
            </div>
          )}

          {/* Error */}
          {msgError && !loadingMessages && (
            <div
              className="flex items-start gap-2 rounded-xl px-4 py-3 text-sm mx-2 my-3"
              style={{ background: 'rgba(248,113,113,0.08)', color: 'var(--red)', border: '1px solid rgba(248,113,113,0.2)' }}
            >
              <AlertCircle size={15} className="mt-0.5 shrink-0" />
              {msgError}
            </div>
          )}

          {/* Load more */}
          {!loadingMessages && hasMore && messages.length > 0 && !searchResults && (
            <div className="flex justify-center mb-4">
              <button
                onClick={loadMore}
                disabled={loadingMore}
                className="flex items-center gap-2 px-4 py-2 rounded-xl text-xs font-medium transition-all"
                style={{
                  background: 'var(--bg-elevated)',
                  border: '1px solid var(--border)',
                  color: 'var(--text-muted)',
                  cursor: loadingMore ? 'wait' : 'pointer',
                }}
              >
                {loadingMore ? (
                  <Loader2 size={12} className="spin" />
                ) : (
                  <ChevronUp size={12} />
                )}
                {loadingMore ? 'Loading…' : 'Load earlier messages'}
              </button>
            </div>
          )}

          {/* Message list */}
          {!loadingMessages &&
            !msgError &&
            filteredMessages.map((msg) => (
              <MessageCard
                key={msg.id}
                message={msg}
                saved={savedIds.has(msg.id)}
                onSave={saveExtract}
                onUnsave={removeExtract}
              />
            ))}

          {/* Empty filtered result */}
          {!loadingMessages &&
            !msgError &&
            selectedChannel &&
            messages.length > 0 &&
            filteredMessages.length === 0 && (
              <div className="flex flex-col items-center justify-center py-16 gap-2">
                <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
                  No messages match this filter
                </p>
                <button
                  onClick={() => setFilter('all')}
                  className="text-xs underline underline-offset-2"
                  style={{ color: 'var(--accent-text)' }}
                >
                  Show all
                </button>
              </div>
            )}

          <div ref={messagesEndRef} />
        </div>
      </div>

      {/* Extract Panel */}
      {showExtracts && (
        <ExtractPanel
          messages={extracts}
          onRemove={removeExtract}
          onClearAll={clearExtracts}
          onClose={() => setShowExtracts(false)}
        />
      )}
    </div>
  );
}
