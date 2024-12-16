import { Component, createEffect, createSignal, onMount, Show } from "solid-js";
import { Title } from "@solidjs/meta";
import { ENV } from "~/const";
import { useUser } from "~/usercontext";
import { get_superuser_data_from_session } from "../utils/userutils";

import WelcomeMessage from "~/components/WelcomeMessage";
import Navbar from "~/components/Navbar";
import styles from './admin.module.css';

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
async function fetch_categories(page_size: number, page_number: number): Promise<Category[] | null> {
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
  return data.message.categories;
}

async function get_categories(page_size: number, page_number: number): Promise<Category[]> {
  const categories = await fetch_categories(page_size, page_number);
  if (categories == null)
    return [];
  return categories;
}

interface Category {
  category_name: string;
  id: number;
}

/*
TODO AND NOTES FOR IMPROVEMENTS
- Filter results in the search bar
- Implement the options container
- Implement the rest of the menus
- Pre-load the lists (e.g. categories, questions) when the user hovers on the menu,
  and then load the data of each said list item when hovering over that specific one.
- Cache the data as well and use a time-stamp in the database (fetch only this) to check if the data is up-to-date.
- Use a debounce function for the search bar to reduce the number of requests made to the server.
- Use memoization for the computed values to avoid unnecessary re-renders.
- Move code into separate files for components (because of course).
*/
const Admin: Component = () => {
  const { userId, setUserId, username, setUsername, superuser, setSuperuser } = useUser();
  const [loading, setLoading] = createSignal(true);
  const [currentmenu, setCurrentmenu] = createSignal(-1);
  const [categories, setCategories] = createSignal<Category[]>([]);

  const menus = ["User", "Question", "Category", "Permission"]; // place holder, will be taken from server at some point
  const PAGE_SIZE = 7; // be 7

  onMount(async () => {
    if (superuser()) {
      setLoading(false);
      return;
    }

    const user_data = await get_superuser_data_from_session();
    /* set local user data for display */
    setUserId(user_data.user_id);
    setUsername(user_data.username);
    setSuperuser(user_data.superuser);
    setLoading(false);
  })

  createEffect(async () => {
    switch (currentmenu()) {
      case 0:
        break;
      case 1:
        break;
      case 2:
        setCategories(await get_categories(PAGE_SIZE, 0));
        break;
      case 3:
        break;
      default:
        break;
    }
  });

  return (
    <>
      <Title>Trivia | Admin</Title>
      <Show when={!loading()} fallback={<AdminPlaceHolder />}>
        <Show when={superuser()} fallback={<div>Unauthorized</div>}>
          <Navbar />
          <div class={styles.page_container} onclick={() => setCurrentmenu(-1)}>
            <div class={styles.side_container}>
              <WelcomeMessage class={styles.welcome_message} admin={true} />
              {menus.map((menu: string, idx: number) => (
                <MenuComponent name={menu} class={() => idx == currentmenu() ? styles.menu_item_selected
                  : styles.menu_item} onclick={(e: MouseEvent) => { e.stopPropagation(); setCurrentmenu(idx) }} />
              ))}
            </div>
            <div class={styles.admin_container}>
              <div class={styles.menu_container}>
                <div class={styles.search_container} onclick={(e: MouseEvent) => { e.stopPropagation() }}>
                  {currentmenu() != -1 && <input class={styles.search_bar} placeholder="Search...">search bar</input>}
                  <div class={currentmenu() != -1 ? styles.result_wrapper : undefined}>
                    {(() => {
                      switch (currentmenu()) {
                        case 0:
                          return <span>Menu 0 Content</span>;
                        case 1:
                          return <span>Menu 1 Content</span>;
                        case 2:
                          return (
                            <>
                              {categories().length === 0 ? (
                                <span>No categories found</span>
                              ) : (
                                categories().map((category: Category) => (
                                  <ListComponent name={category.category_name} id={category.id} class={styles.list_item} />
                                ))
                              )}
                            </>
                          );
                        case 3:
                          return <span>Menu 3 content</span>;
                        default:
                          return <span>Select a menu to get started.</span>;
                      }
                    })()}
                  </div>
                </div>
                <div class={styles.options_container} onclick={(e: MouseEvent) => { e.stopPropagation() }}>

                </div>
              </div>
            </div>
          </div>
        </Show>
      </Show>
    </>
  )
}

const AdminPlaceHolder = () => {
  const placeholder_menus = Array(4).fill("​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​");

  return (
    <div class={styles.page_container}>
      <Navbar placeholder={true} />
      <div class={styles.side_container}>
        <WelcomeMessage class={styles.welcome_message} admin={true} />
        {placeholder_menus.map((menu: string, idx: number) => (
          <MenuComponent name={menu} class={() => styles.menu_item_placeholder} />
        ))}
      </div>
      <div class={styles.admin_container}>
        <div class={styles.menu_container}>
          <div class={styles.search_container} />
          <div class={styles.options_container} />
        </div>
      </div>
    </div>
  );
}

interface MenuComponentProps {
  name: string;
  class?: () => string;
  onclick?: (e: MouseEvent) => void;
}

const MenuComponent = ({ name, class: class_, onclick: onclick_ }: MenuComponentProps) => {
  return (
    <div class={class_!()} onclick={onclick_}>
      {name}
    </div>
  );
}

interface ListComponentProps {
  name: string;
  id: number;
  class?: string;
  onclick?: (e: MouseEvent) => void;
}

const ListComponent = ({ name, id, class: class_, onclick: onclick_ }: ListComponentProps) => {
  return (
    <div class={class_} onclick={onclick_}>
      {name}
    </div>
  );
}

export default Admin;