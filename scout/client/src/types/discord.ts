export interface CurrentUser {
  id: string;
  username: string;
  discriminator: string;
  avatar: string | null;
  global_name: string | null;
}

export interface Guild {
  id: string;
  name: string;
  icon: string | null;
  owner: boolean;
  permissions: string;
}

export interface Channel {
  id: string;
  type: number; // 0=text, 2=voice, 4=category, 5=announcement
  name: string;
  position: number;
  parent_id: string | null;
  topic: string | null;
  nsfw?: boolean;
}

export interface MessageAuthor {
  id: string;
  username: string;
  discriminator: string;
  avatar: string | null;
  global_name: string | null;
}

export interface Attachment {
  id: string;
  filename: string;
  size: number;
  url: string;
  proxy_url: string;
  content_type?: string;
  width?: number;
  height?: number;
}

export interface Message {
  id: string;
  content: string;
  timestamp: string;
  edited_timestamp: string | null;
  author: MessageAuthor;
  attachments: Attachment[];
  embeds: unknown[];
  mentions: unknown[];
  type: number;
}

export interface SearchResult {
  messages: Message[][];
  total_results: number;
}

// ─── Filters ─────────────────────────────────────────────────────────────────

export type FilterType = 'all' | 'code' | 'offsets' | 'files' | 'links';

export const OFFSET_RE = /\b0x[0-9A-Fa-f]{3,}\b/;
export const LINK_RE = /https?:\/\/\S+/;
export const CODE_BLOCK_RE = /```[\s\S]*?```|`[^`]+`/;

export function matchesFilter(msg: Message, filter: FilterType): boolean {
  if (filter === 'all') return true;
  if (filter === 'code') return CODE_BLOCK_RE.test(msg.content);
  if (filter === 'offsets') return OFFSET_RE.test(msg.content);
  if (filter === 'files') return msg.attachments.length > 0;
  if (filter === 'links') return LINK_RE.test(msg.content);
  return true;
}
