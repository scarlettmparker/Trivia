import { ENV } from "~/const";
import { Category } from "./const";

/**
 * Helper function to insert a question into the database.
 * Requires permission question.put to be set in the server.
 * Otherwise the request will be rejected and the user will receive 401 Unauthorized.
 * 
 * @param question Question to insert.
 * @param answers Answers to the question.
 * @param category_id Category ID of the question.
 * @param correct_answer Correct answer index.
 */
async function insert_question(question: string, answers: string[], category_id: number, correct_answer: number) {
  const response = await fetch(
    `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/question`,
    {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*',
        'Connection': 'keep-alive',
      },
      credentials: 'include',
      body: JSON.stringify({
        question: question,
        answers: answers,
        category_id: category_id,
        correct_answer: correct_answer
      })
    }
  )

  const data = await response.json();
  return data.message;
}

/**
 * Helper function to fetch categories from the server.
 * Requires permission superuser or category.admin to be retrieved.
 * Otherwise the request will be rejected and the user will receive 401 Unauthorized.
 * This is used to display the categories in the admin panel, and returns a list of categories with their ids.
 * 
 * @param page_size Number of categories to retrieve.
 * @param page_number Page number to retrieve.
 * @returns List of categories with their ids.
 */
async function fetch_categories(page_size: number, page_number: number, CATEGORIES_KEY: string): Promise<Category[] | null> {
  const has_new_data = await get_last_modified("Category", "CATEGORY_LAST_MODIFIED");

  if (!has_new_data && localStorage.getItem(CATEGORIES_KEY)) {
    return JSON.parse(localStorage.getItem(CATEGORIES_KEY)!);
  }

  const response = await fetch(
    `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/category?superuser=true&page_size=${page_size}&page=${page_number}`,
    {
      method: 'GET',
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*',
        'Connection': 'keep-alive',
      },
      credentials: 'include',
    }
  )

  const data = await response.json();
  if (data.message == "No categories found")
    return null;

  localStorage.setItem(CATEGORIES_KEY, JSON.stringify(data.message.categories));
  return data.message.categories;
}

const CACHE_TIMEOUT = 5000;
const memory_cache: { [key: string]: { value: string; timestamp: number } } = {};

/**
 * Helper function to check if the data in the database has been modified since the last time it was fetched.
 * This is used to avoid fetching the same data multiple times, and only fetch it when it has been updated.
 * 
 * @param table_name Name of the table to check for last modified date.
 * @param LAST_MODIFIED_KEY Key to store the last modified date in the local storage.
 */
async function get_last_modified(table_name: string, LAST_MODIFIED_KEY: string): Promise<boolean> {
  const now = Date.now();
  const cached = memory_cache[LAST_MODIFIED_KEY];
  if (cached && (now - cached.timestamp) < CACHE_TIMEOUT) {
    return cached.value !== localStorage.getItem(LAST_MODIFIED_KEY);
  }

  try {
    const controller = new AbortController();
    const timeout_id = setTimeout(() => controller.abort(), 5000);

    const response = await fetch(
      `http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/last_modified?table_name=${table_name}`,
      {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*',
        },
        signal: controller.signal,
        credentials: 'include',
      }
    );

    clearTimeout(timeout_id);

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();
    const last_modified = data.message.last_modified;
    memory_cache[LAST_MODIFIED_KEY] = {
      value: last_modified,
      timestamp: now
    };

    if (!localStorage.getItem(LAST_MODIFIED_KEY)) {
      localStorage.setItem(LAST_MODIFIED_KEY, last_modified);
      return true;
    }

    if (last_modified !== localStorage.getItem(LAST_MODIFIED_KEY)) {
      localStorage.setItem(LAST_MODIFIED_KEY, last_modified);
      return true;
    }

    return false;
  } catch (error) {
    console.error('Error fetching last modified:', error);
    return false;
  }
}

async function get_categories(page_size: number, page_number: number): Promise<Category[]> {
  const categories = await fetch_categories(page_size, page_number, "CATEGORIES");
  return categories ?? [];
}

/**
 * Helper function to get the data for the selected menu.
 * This is used to display the content of the selected menu in the admin panel.
 * 
 * @param endpoint Selected menu endpoint.
 */
export async function get_data(endpoint: number, page_size: number, page_number: number) {
  switch (endpoint) {
    case 0:
      return { message: "Menu 0 Content" };
    case 1:
      return { message: "Menu 1 Content" };
    case 2:
      return await get_categories(page_size, page_number);
    case 3:
      return { message: "Menu 3 Content" };
    default:
      return { message: "Select a menu to get started." };
  }
}

export default null;