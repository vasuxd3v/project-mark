import express, { Request, Response } from 'express';
import cors from 'cors';
import { discordFetch } from './discord';

const app = express();
const PORT = parseInt(process.env.PORT ?? '3001', 10);

app.use(cors({ origin: 'http://localhost:5173' }));
app.use(express.json());

// ─── Helpers ──────────────────────────────────────────────────────────────────

function getToken(req: Request): string | null {
  const t = req.headers['x-discord-token'];
  return typeof t === 'string' && t.length > 0 ? t : null;
}

async function proxy(req: Request, res: Response, path: string): Promise<void> {
  const token = getToken(req);
  if (!token) {
    res.status(401).json({ error: 'Missing X-Discord-Token header' });
    return;
  }
  try {
    const upstream = await discordFetch(path, token);
    const body: unknown = await upstream.json();
    res.status(upstream.status).json(body);
  } catch (err) {
    console.error('[proxy error]', path, err);
    res.status(502).json({ error: 'Discord request failed' });
  }
}

// ─── Routes ───────────────────────────────────────────────────────────────────

// Validate token and return current user
app.get('/api/me', (req, res) => proxy(req, res, '/users/@me'));

// All guilds (servers) the user is in
app.get('/api/guilds', (req, res) => proxy(req, res, '/users/@me/guilds'));

// All channels in a guild
app.get('/api/guilds/:id/channels', (req, res) =>
  proxy(req, res, `/guilds/${req.params.id}/channels`),
);

// Messages in a channel (paginated)
app.get('/api/channels/:id/messages', (req, res) => {
  const { limit = '50', before } = req.query as Record<string, string>;
  const qs = new URLSearchParams({ limit: String(Math.min(Number(limit), 100)) });
  if (before) qs.set('before', before);
  return proxy(req, res, `/channels/${req.params.id}/messages?${qs}`);
});

// Search messages in a guild (with optional channel filter)
app.get('/api/guilds/:id/search', (req, res) => {
  const { q, channel_id } = req.query as Record<string, string>;
  const qs = new URLSearchParams();
  if (q) qs.set('content', q);
  if (channel_id) qs.set('channel_id', channel_id);
  return proxy(req, res, `/guilds/${req.params.id}/messages/search?${qs}`);
});

// ─── Start ────────────────────────────────────────────────────────────────────

app.listen(PORT, () => {
  console.log(`[scout] Server → http://localhost:${PORT}`);
});
