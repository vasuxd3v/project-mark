const BASE = 'https://discord.com/api/v10';
const UA =
  'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36';

export async function discordFetch(
  path: string,
  token: string,
  options: RequestInit = {},
): Promise<Response> {
  return fetch(`${BASE}${path}`, {
    ...options,
    headers: {
      Authorization: token,
      'User-Agent': UA,
      'Content-Type': 'application/json',
      ...options.headers,
    },
  });
}
