import 'dotenv/config';
import Anthropic from '@anthropic-ai/sdk';
import express, { Request, Response } from 'express';
import cors from 'cors';
import { discordFetch } from './discord';

const app = express();
const PORT = parseInt(process.env.PORT ?? '3001', 10);

app.use(cors({ origin: 'http://localhost:5173' }));
app.use(express.json());

const anthropic = process.env.ANTHROPIC_API_KEY
  ? new Anthropic({ apiKey: process.env.ANTHROPIC_API_KEY })
  : null;

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

// ─── Discord Routes ───────────────────────────────────────────────────────────

app.get('/api/me', (req, res) => proxy(req, res, '/users/@me'));

app.get('/api/guilds', (req, res) => proxy(req, res, '/users/@me/guilds'));

app.get('/api/guilds/:id/channels', (req, res) =>
  proxy(req, res, `/guilds/${req.params.id}/channels`),
);

app.get('/api/channels/:id/messages', (req, res) => {
  const { limit = '50', before } = req.query as Record<string, string>;
  const qs = new URLSearchParams({ limit: String(Math.min(Number(limit), 100)) });
  if (before) qs.set('before', before);
  return proxy(req, res, `/channels/${req.params.id}/messages?${qs}`);
});

app.get('/api/guilds/:id/search', (req, res) => {
  const { q, channel_id } = req.query as Record<string, string>;
  const qs = new URLSearchParams();
  if (q) qs.set('content', q);
  if (channel_id) qs.set('channel_id', channel_id);
  return proxy(req, res, `/guilds/${req.params.id}/messages/search?${qs}`);
});

// ─── AI Channel Analysis ──────────────────────────────────────────────────────

interface ChannelInput {
  id: string;
  name: string;
  topic?: string | null;
}

app.post('/api/guilds/:id/analyze', async (req: Request, res: Response): Promise<void> => {
  if (!anthropic) {
    res.status(503).json({ error: 'ANTHROPIC_API_KEY not configured' });
    return;
  }

  const { channels } = req.body as { channels: ChannelInput[] };
  if (!Array.isArray(channels) || channels.length === 0) {
    res.status(400).json({ error: 'channels array required' });
    return;
  }

  const channelList = channels
    .map((c) => `- id:${c.id}  name:#${c.name}${c.topic ? `  topic:"${c.topic}"` : ''}`)
    .join('\n');

  const prompt = `You are helping a developer who does reverse engineering and systems programming research on Roblox internals. They are browsing a Discord server and want to know which channels are most likely to contain useful technical content like:
- Memory offsets and addresses (e.g. 0x1A2B3C)
- Code snippets, patches, scripts
- Binary analysis findings
- Struct layouts, VM internals
- Tool releases or resources

Rate each channel as one of:
- "hot"  → very likely has dev/RE content based on name or topic
- "warm" → might have some useful content occasionally
- "cold" → general chat, off-topic, or irrelevant (memes, vc, introductions, etc.)

Channels:
${channelList}

Reply ONLY with a compact JSON object mapping each channel id to its rating.
Example: {"123":"hot","456":"cold","789":"warm"}`;

  try {
    const message = await anthropic.messages.create({
      model: 'claude-haiku-4-5-20251001',
      max_tokens: 512,
      messages: [{ role: 'user', content: prompt }],
    });

    const text = message.content[0].type === 'text' ? message.content[0].text : '{}';
    const jsonMatch = text.match(/\{[^}]+\}/);
    const result = JSON.parse(jsonMatch?.[0] ?? '{}') as Record<string, string>;
    res.json(result);
  } catch (err) {
    console.error('[analyze]', err);
    res.status(500).json({ error: 'AI analysis failed' });
  }
});

// ─── Start ────────────────────────────────────────────────────────────────────

app.listen(PORT, () => {
  console.log(`[scout] Server → http://localhost:${PORT}`);
  if (!anthropic) {
    console.warn('[scout] ANTHROPIC_API_KEY not set — AI channel analysis disabled');
  }
});
