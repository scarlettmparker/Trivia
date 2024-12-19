export interface CategoryData {
  default: Category[];
  filters: {
    newest: number[];
    oldest: number[];
    "a-z": number[];
    "z-a": number[];
  };
  search: {
    [key: string]: number[];
  };
}

export interface Category {
  category_name: string;
  id: number;
}

export const CACHE_TIMEOUT = 5000;
export const memory_cache: {
  [key: string]: {
    value: string;
    timestamp: number
  }
} = {};

export default null;