export const ENV = {
  VITE_SERVER_HOST: import.meta.env.VITE_SERVER_HOST,
  VITE_SERVER_PORT: import.meta.env.VITE_SERVER_PORT,
};

export interface UserData {
  user_id: number;
  username: string;
}

export interface CacheData {
  timestamp: number;
  data: UserData;
}