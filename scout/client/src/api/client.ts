import type { CurrentUser, Guild, Channel, Message, SearchResult } from '../types/discord';

let _token: string | null = null;

export function setApiToken(t: string | null): void {
  _token = t;
}

async function request<T>(path: string, options?: RequestInit): Promise<T> {
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options?.headers as Record<string, string>),
  };
  if (_token) headers['X-Discord-Token'] = _token;

  const res = await fetch(path, { ...options, headers });
  if (!res.ok) {
    const body = (await res.json().catch(() => ({}))) as { message?: string; error?: string };
    throw new Error(body.message ?? body.error ?? `HTTP ${res.status}`);
  }
  return res.json() as Promise<T>;
}

export const getMe = () => request<CurrentUser>('/api/me');

export const getGuilds = () => request<Guild[]>('/api/guilds');

export const getChannels = (guildId: string) =>
  request<Channel[]>(`/api/guilds/${guildId}/channels`);

export const getMessages = (channelId: string, before?: string) => {
  const qs = new URLSearchParams({ limit: '50' });
  if (before) qs.set('before', before);
  return request<Message[]>(`/api/channels/${channelId}/messages?${qs}`);
};

export const searchMessages = (guildId: string, channelId: string, q: string) =>
  request<SearchResult>(
    `/api/guilds/${guildId}/search?channel_id=${channelId}&q=${encodeURIComponent(q)}`,
  );
