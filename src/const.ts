export const CACHE_DURATION = 5 * 60 * 1000;
export const CACHE_KEY = 'user_data_cache';
export const SUPER_CACHE_KEY = 'superuser_data_cache';

export const MAX_RETRIES = 3;
export const BASE_DELAY = 1000;

export const ENV = {
  VITE_SERVER_HOST: import.meta.env.VITE_SERVER_HOST,
  VITE_SERVER_PORT: import.meta.env.VITE_SERVER_PORT,
};

export interface SuperUserData {
  user_id: number;
  username: string;
  superuser: boolean;
}

export interface SuperCacheData {
  timestamp: number;
  data: SuperUserData;
}

export interface UserData {
  user_id: number;
  username: string;
}

export interface CacheData {
  timestamp: number;
  data: UserData;
}