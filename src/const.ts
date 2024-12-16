export const CACHE_DURATION = 5 * 60 * 1000;
export const CACHE_KEY = 'user_data_cache';

export const ENV = {
  VITE_SERVER_HOST: import.meta.env.VITE_SERVER_HOST,
  VITE_SERVER_PORT: import.meta.env.VITE_SERVER_PORT,
};

export interface SuperUserData {
  user_id: number;
  username: string;
  superuser: boolean;
}

export interface UserData {
  user_id: number;
  username: string;
}

export interface CacheData {
  timestamp: number;
  data: UserData;
}