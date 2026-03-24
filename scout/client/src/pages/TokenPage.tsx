import { useEffect, useRef, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Eye, EyeOff, Crosshair, ArrowRight, AlertCircle, CheckCircle2 } from 'lucide-react';
import { getMe } from '../api/client';
import { setApiToken } from '../api/client';
import { useAuth } from '../App';

export default function TokenPage() {
  const { token, login } = useAuth();
  const navigate = useNavigate();
  const inputRef = useRef<HTMLInputElement>(null);

  const [value, setValue] = useState('');
  const [show, setShow] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [validating, setValidating] = useState(false);

  // If token already saved, try to auto-validate
  useEffect(() => {
    if (!token) return;
    setValidating(true);
    getMe()
      .then((user) => {
        login(token, user);
        navigate('/servers', { replace: true });
      })
      .catch(() => {
        // Token invalid — stay on login page
        localStorage.removeItem('scout_token');
        setApiToken(null);
      })
      .finally(() => setValidating(false));
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    const t = value.trim();
    if (!t) return;

    setLoading(true);
    setError(null);

    try {
      setApiToken(t);
      const user = await getMe();
      login(t, user);
      navigate('/servers');
    } catch {
      setApiToken(null);
      setError('Invalid token or network error. Check your token and try again.');
    } finally {
      setLoading(false);
    }
  }

  if (validating) {
    return (
      <div
        className="min-h-screen flex items-center justify-center"
        style={{ background: 'var(--bg-base)' }}
      >
        <div className="flex flex-col items-center gap-4">
          <div
            className="w-8 h-8 rounded-full border-2 border-t-transparent spin"
            style={{ borderColor: 'var(--accent)', borderTopColor: 'transparent' }}
          />
          <span style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
            Validating saved token…
          </span>
        </div>
      </div>
    );
  }

  return (
    <div
      className="min-h-screen flex items-center justify-center p-4"
      style={{ background: 'var(--bg-base)' }}
    >
      {/* Ambient glow */}
      <div
        className="pointer-events-none fixed inset-0"
        style={{
          background:
            'radial-gradient(ellipse 60% 40% at 50% 20%, rgba(124,58,237,0.08) 0%, transparent 70%)',
        }}
      />

      <div className="w-full max-w-md relative">
        {/* Logo */}
        <div className="text-center mb-10">
          <div className="inline-flex items-center justify-center mb-4">
            <div
              className="w-14 h-14 rounded-2xl flex items-center justify-center"
              style={{
                background: 'var(--bg-elevated)',
                border: '1px solid var(--border)',
                boxShadow: '0 0 32px var(--accent-glow)',
              }}
            >
              <Crosshair
                size={28}
                style={{ color: 'var(--accent-text)' }}
                strokeWidth={1.5}
              />
            </div>
          </div>
          <h1
            className="text-3xl font-bold tracking-tight"
            style={{ color: 'var(--text)', letterSpacing: '-0.02em' }}
          >
            Scout
          </h1>
          <p className="mt-2 text-sm" style={{ color: 'var(--text-muted)' }}>
            Discord Intelligence Dashboard
          </p>
        </div>

        {/* Card */}
        <div
          className="rounded-2xl p-8"
          style={{
            background: 'var(--bg-surface)',
            border: '1px solid var(--border)',
            boxShadow: '0 24px 64px rgba(0,0,0,0.4)',
          }}
        >
          <form onSubmit={handleSubmit} className="flex flex-col gap-5">
            <div>
              <label
                htmlFor="token"
                className="block text-sm font-medium mb-2"
                style={{ color: 'var(--text-muted)' }}
              >
                Your Discord Token
              </label>

              <div className="relative">
                <input
                  ref={inputRef}
                  id="token"
                  type={show ? 'text' : 'password'}
                  value={value}
                  onChange={(e) => {
                    setValue(e.target.value);
                    setError(null);
                  }}
                  placeholder="Paste your token here…"
                  autoComplete="off"
                  spellCheck={false}
                  className="w-full rounded-xl px-4 py-3 pr-12 text-sm font-mono outline-none transition-all"
                  style={{
                    background: 'var(--bg-elevated)',
                    border: `1px solid ${error ? 'var(--red)' : 'var(--border)'}`,
                    color: 'var(--text)',
                    caretColor: 'var(--accent-text)',
                  }}
                  onFocus={(e) =>
                    (e.currentTarget.style.borderColor = error ? 'var(--red)' : 'var(--accent)')
                  }
                  onBlur={(e) =>
                    (e.currentTarget.style.borderColor = error ? 'var(--red)' : 'var(--border)')
                  }
                />
                <button
                  type="button"
                  onClick={() => setShow((s) => !s)}
                  className="absolute right-3 top-1/2 -translate-y-1/2 p-1 rounded transition-colors"
                  style={{ color: 'var(--text-dim)' }}
                  tabIndex={-1}
                >
                  {show ? <EyeOff size={16} /> : <Eye size={16} />}
                </button>
              </div>
            </div>

            {/* Error */}
            {error && (
              <div
                className="flex items-start gap-2 rounded-xl px-4 py-3 text-sm"
                style={{ background: 'rgba(248,113,113,0.08)', color: 'var(--red)' }}
              >
                <AlertCircle size={15} className="mt-0.5 shrink-0" />
                {error}
              </div>
            )}

            {/* Submit */}
            <button
              type="submit"
              disabled={loading || !value.trim()}
              className="flex items-center justify-center gap-2 w-full rounded-xl py-3 text-sm font-semibold transition-all"
              style={{
                background: loading || !value.trim() ? 'var(--bg-elevated)' : 'var(--accent)',
                color: loading || !value.trim() ? 'var(--text-dim)' : '#fff',
                cursor: loading || !value.trim() ? 'not-allowed' : 'pointer',
                border: `1px solid ${loading || !value.trim() ? 'var(--border)' : 'var(--accent)'}`,
              }}
            >
              {loading ? (
                <>
                  <div
                    className="w-4 h-4 rounded-full border-2 border-t-transparent spin"
                    style={{ borderColor: 'rgba(255,255,255,0.4)', borderTopColor: 'transparent' }}
                  />
                  Connecting…
                </>
              ) : (
                <>
                  Connect
                  <ArrowRight size={15} />
                </>
              )}
            </button>
          </form>

          {/* Footer note */}
          <div className="mt-6 flex items-center gap-2 text-xs" style={{ color: 'var(--text-dim)' }}>
            <CheckCircle2 size={12} />
            Token is stored locally in your browser. Never sent anywhere except Discord's API.
          </div>
        </div>

        {/* How to get token */}
        <div
          className="mt-4 rounded-xl px-5 py-4 text-xs leading-relaxed"
          style={{
            background: 'var(--bg-surface)',
            border: '1px solid var(--border-dim)',
            color: 'var(--text-dim)',
          }}
        >
          <span style={{ color: 'var(--text-muted)', fontWeight: 600 }}>How to get your token:</span>{' '}
          Open Discord in browser → DevTools (F12) → Network tab → send any message → find a request
          → copy the <code style={{ color: 'var(--accent-text)' }}>Authorization</code> header value.
        </div>
      </div>
    </div>
  );
}
