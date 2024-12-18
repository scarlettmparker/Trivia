import { UserData, CacheData, ENV, CACHE_KEY, BASE_DELAY, MAX_RETRIES, SuperUserData, SuperCacheData } from "~/const";

/**
 * Delay the execution of the function.
 */
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

/**
 * Get the user data from the session.
 * @returns UserData object.
 */
function get_cached_user_data(CACHE_DURATION: number, CACHE_KEY: string): UserData | null {
  const cached_data = localStorage.getItem(CACHE_KEY);

  if (!cached_data) return null;

  const cache: CacheData = JSON.parse(cached_data);
  if (Date.now() - cache.timestamp < CACHE_DURATION) {
    return cache.data;
  }

  return null;
}

/**
 * Fetch the user data from the server.
 * This function sends a GET request to the server to get the user data.
 * @param CACHE_KEY Cache key to store the user data.
 * @returns UserData object.
 */
async function fetch_user_data(CACHE_KEY: string): Promise<UserData | null> {
  const REQUEST_TIMEOUT = 1000;

  for (let attempt = 0; attempt <= MAX_RETRIES; attempt++) {
    const controller = new AbortController();
    const timeout_id = setTimeout(() => controller.abort(), REQUEST_TIMEOUT);
    try {
      const response = await fetch(
        `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/session`,
        {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Connection': 'keep-alive',
          },
          credentials: 'include',
          signal: controller.signal,
        }
      );
      if (response.status == 401) {
        console.error('Unauthorized');
        return null;
      }
      const data = await response.json();
      if (data.status === 'ok') {
        const user_data = data.message as UserData;
        localStorage.setItem(
          CACHE_KEY,
          JSON.stringify({ data: user_data, timestamp: Date.now() })
        );
        return user_data;
      }
    } catch (error) {
      console.error(`Attempt ${attempt + 1}/${MAX_RETRIES + 1} failed:`, error);
      if (attempt < MAX_RETRIES) {
        const backoff_delay = BASE_DELAY * Math.pow(2, attempt);
        await delay(backoff_delay);
      }
    } finally {
      clearTimeout(timeout_id);
    }
  }
  return null;
}

/**
 * Get the superuser data from the cache.
 * @returns SuperUserData object.
 */
function get_cached_superuser_data(CACHE_DURATION: number, CACHE_KEY: string): SuperUserData | null {
  const cached_data = localStorage.getItem(CACHE_KEY);

  if (!cached_data) return null;

  const cache: SuperCacheData = JSON.parse(cached_data);
  if (Date.now() - cache.timestamp < CACHE_DURATION) {
    return cache.data;
  }

  return null;
}

/**
 * Fetch the superuser data from the server.
 * This function sends a GET request to the server to get the superuser data.
 * It will return a boolean for "superuser" along with the regular user data.
 * This is to ensure that normal users cannot access the admin panel.
 * @returns SuperUserData object.
 */
async function fetch_superuser_data(CACHE_KEY: string): Promise<SuperUserData | null> {
  const REQUEST_TIMEOUT = 1000;

  for (let attempt = 0; attempt <= MAX_RETRIES; attempt++) {
    const controller = new AbortController();
    const timeout_id = setTimeout(() => controller.abort(), REQUEST_TIMEOUT)
    try {
      const response = await fetch(
        `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/session?superuser=true`,
        {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Connection': 'keep-alive',
          },
          credentials: 'include',
          signal: controller.signal,
        }
      );
      if (response.status == 401) {
        console.error('Unauthorized');
        return null;
      }
      const data = await response.json();
      if (data.status == 'ok') {
        const user_data = data.message as SuperUserData;
        localStorage.setItem(
          CACHE_KEY,
          JSON.stringify({ data: user_data, timestamp: Date.now() })
        );
        return user_data;
      }
    } catch (error) {
      console.error(`Attempt ${attempt + 1}/${MAX_RETRIES + 1} failed:`, error);
      if (attempt < MAX_RETRIES) {
        const backoff_delay = BASE_DELAY * Math.pow(2, attempt);
        await delay(backoff_delay);
      }
    } finally {
      clearTimeout(timeout_id);
    }
  }
  return null;
}

/**
 * Get the user data from the session.
 * @returns UserData object containing username and user ID.
 */
export async function get_user_data_from_session(CACHE_DURATION: number, CACHE_KEY: string): Promise<UserData> {
  const cached_data = get_cached_user_data(CACHE_DURATION, CACHE_KEY);
  if (cached_data) return cached_data;

  const fetched_data = await fetch_user_data(CACHE_KEY);
  if (fetched_data) return fetched_data;

  return { user_id: -1, username: "" };
}

/**
 * Get the superuser data from the session.
 * @returns SuperUserData object containing username, user ID, and superuser status.
 */
export async function get_superuser_data_from_session(CACHE_DURATION: number, CACHE_KEY: string): Promise<SuperUserData> {
  const cached_data = get_cached_superuser_data(CACHE_DURATION, CACHE_KEY);
  if (cached_data) return cached_data;

  const fetched_data = await fetch_superuser_data(CACHE_KEY);
  if (fetched_data) return fetched_data;

  return { user_id: -1, username: "", superuser: false};
}

/**
 * Logout the user. This function sends a POST request to the server to log out the user.
 * It also clears the cache and cookies of the user/session data.
 * 
 * @param userId User ID.
 * @param setUserId Function to set the user ID.
 * @param setUsername Function to set the username.
 */
export async function logout(userId: number, setUserId: (id: number) => void, setUsername: (name: string) => void) {
  const response = await fetch(
    `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/logout`,
    {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*',
      },
      body: JSON.stringify({ user_id: userId }),
      credentials: 'include',
    }
  );

  const data = await response.json();
  if (data.status == 'ok') {
    localStorage.removeItem(CACHE_KEY);
    document.cookie.split(";").forEach(cookie => {
      document.cookie = cookie.replace(/^ +/, "").replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/");
    });
    
    setUserId(-1);
    setUsername("");
    return true;
  } else { // just debug for now
    console.error('Failed to log out:', data.message);
  }
  return false;
}

export default null;