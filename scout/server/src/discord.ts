const BASE = 'https://discord.com/api/v10';
const UA =
  'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36';

export async function discordFetch(
  path: string,
  token: string,
  options: RequestInit = {},
  maxRetries = 3,
): Promise<Response> {
  for (let attempt = 0; attempt <= maxRetries; attempt++) {
    const res = await fetch(`${BASE}${path}`, {
      ...options,
      headers: {
        Authorization: token,
        'User-Agent': UA,
        'Content-Type': 'application/json',
        ...options.headers,
      },
    });

    if (res.status === 429 && attempt < maxRetries) {
      const body = (await res.clone().json().catch(() => ({ retry_after: 1 }))) as {
        retry_after?: number;
        global?: boolean;
      };
      const wait = Math.ceil((body.retry_after ?? 1) * 1000);
      console.log(`[scout] Rate limited on ${path} — retrying in ${wait}ms (attempt ${attempt + 1})`);
      await new Promise((r) => setTimeout(r, wait));
      continue;
    }

    return res;
  }
  throw new Error('Max retries exceeded after rate limiting');
}
