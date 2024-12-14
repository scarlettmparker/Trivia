import { createSignal, onMount, type Component } from 'solid-js';
import { useUser } from '~/usercontext';

const ENV = {
  VITE_SERVER_HOST: import.meta.env.VITE_SERVER_HOST,
  VITE_SERVER_PORT: import.meta.env.VITE_SERVER_PORT,
};

interface UserData {
  user_id: number;
  username: string;
}

interface CacheData {
  timestamp: number;
  data: UserData;
}

const CACHE_DURATION = 5 * 60 * 1000;
const CACHE_KEY = 'user_data_cache';

/**
 * Get the user data from the session.
 * @returns UserData object.
 */
async function get_user_data_from_session() {
  const cached_data = localStorage.getItem(CACHE_KEY);

  if (cached_data) {
    const cache: CacheData = JSON.parse(cached_data);
    if (Date.now() - cache.timestamp < CACHE_DURATION) {
      return cache.data;
    }
  }

  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 5000);

  try {
    const response = await fetch(`http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/session`, {
      method: 'GET',
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*',
      },
      credentials: 'include',
      signal: controller.signal,
    });

    const data = await response.json();
    if (data.status === 'ok') {
      const user_data = data.message as UserData;
      localStorage.setItem(CACHE_KEY, JSON.stringify({ data: user_data, timestamp: Date.now() }));
      return user_data;
    }
  } catch (error) {
    console.error('Failed to fetch user data:', error);
  } finally {
    clearTimeout(timeoutId);
  }
  return { user_id: -1, username: "" };
}

const Index: Component = () => {
  const { userId, setUserId, username, setUsername } = useUser();

  onMount(async () => {
    if (userId() !== -1) return;

    const user_data = await get_user_data_from_session();
    setUserId(user_data.user_id);
    setUsername(user_data.username);
  });

  return (
    <div>
      {username() == "" ? <>Hello, guest!</> : <div>Welcome back, {username()}!</div>}
    </div>
  );
};

export default Index;