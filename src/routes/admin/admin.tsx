import { Component, createEffect, createSignal, JSX, onMount, Show } from "solid-js";
import { Title } from "@solidjs/meta";
import { useUser } from "~/usercontext";
import { CACHE_DURATION, SUPER_CACHE_KEY } from "~/const";
import { get_superuser_data_from_session } from "../utils/userutils";
import { Category, CategoryData } from "./utils/const";
import { get_data } from "./utils/endpoint_utils";

import WelcomeMessage from "~/components/WelcomeMessage";
import Navbar from "~/components/Navbar";
import styles from './admin.module.css';
import AdminPlaceHolder from "~/components/AdminPlaceholder";
import MenuComponent from "~/components/MenuComponent";
import ListComponent from "~/components/ListComponent";

/*
TODO AND NOTES FOR IMPROVEMENTS
- Filter results in the search bar
- Implement the options container
- Implement the rest of the menus

- Use a debounce function for the search bar to reduce the number of requests made to the server.
- Use memoization for the computed values to avoid unnecessary re-renders.
- Move code into separate files for components (because of course).
*/
const Admin: Component = () => {
  const { userId, setUserId, username, setUsername, superuser, setSuperuser } = useUser();
  const [loading, setLoading] = createSignal(true);
  const [currentmenu, setCurrentmenu] = createSignal(-1);
  const [page, setPage] = createSignal(0);

  const [categories, setCategories] = createSignal<Category[]>([]);
  const [currentlistmenu, setCurrentlistmenu] = createSignal(-1);

  /* placeholders, will be replaced with actual data soon */
  const menus = ["User", "Question", "Category", "Permission"];
  const category_permissions = ["Create", "Update", "Delete"];
  
  const PAGE_SIZE = 7;


  onMount(async () => {
    if (superuser()) {
      setLoading(false);
      return;
    }

    const user_data = await get_superuser_data_from_session(CACHE_DURATION, SUPER_CACHE_KEY);
    /* set local user data for display */
    setUserId(user_data.user_id);
    setUsername(user_data.username);
    setSuperuser(user_data.superuser);
    setLoading(false);
  })

  createEffect(() => {
    /* when menu changes, do something */
  })

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
                  : styles.menu_item} onclick={(e: MouseEvent) => { 
                    e.stopPropagation(); 
                    setCurrentmenu(idx);
                    setCurrentlistmenu(-1); 
                  }}
                  onmouseover={async () => {
                    if (idx == 2 && categories().length === 0) {
                      const result = await get_data(idx, PAGE_SIZE, page()).then((data) => data as CategoryData);
                      const categories = result.default;
                      setCategories(categories as Category[]);
                    }
                  }} />
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
                                <MenuDataComponent items={categories()}>
                                  {(category) => (
                                    <ListComponent name={category.category_name} id={category.id}
                                      currentmenu={currentlistmenu} onclick={() => setCurrentlistmenu(category.id)} />
                                  )}
                                </MenuDataComponent>
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

// will not be final, just a placeholder for now
interface User {
  username: string;
  id: number;
}

interface Question {
  question: string;
  id: number;
}

interface Permission {
  permission: string;
  id: number;
}

type MenuItem = Category | User | Question | Permission;

interface MenuDataProps<T extends MenuItem> {
  items: T[];
  children: (item: T) => JSX.Element;
}

const MenuDataComponent = <T extends MenuItem>({ items, children }: MenuDataProps<T>) => {
  return (
    <>
      {items.map((item) => children(item))}
    </>
  );
};

export default Admin;