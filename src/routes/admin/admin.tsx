import { Component, createSignal, onMount, Show } from "solid-js";
import { Title } from "@solidjs/meta";
import { useUser } from "~/usercontext";
import { get_superuser_data_from_session } from "../utils/userutils";
import WelcomeMessage from "~/components/WelcomeMessage";
import styles from './admin.module.css';
import Navbar from "~/components/Navbar";

const Admin: Component = () => {
  const { userId, setUserId, username, setUsername, superuser, setSuperuser } = useUser();
  const [loading, setLoading] = createSignal(true);
  const [currentmenu, setCurrentmenu] = createSignal(-1);

  const menus = ["User", "Question", "Category", "Permission"];

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
                <div class={styles.search_container}>

                </div>
                <div class={styles.options_container}>

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
  const placeholder_menus = Array(5).fill("​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​");
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

export default Admin;