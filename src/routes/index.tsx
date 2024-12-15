import { createSignal, onMount, type Component } from 'solid-js';
import { CACHE_DURATION, CACHE_KEY } from '~/const';
import { useUser } from '~/usercontext';
import { get_user_data_from_session } from './utils/userutils';
import styles from './index.module.css';
import FilterMenu from '~/components/FilterMenu';

/**
 * Helper function to get the local time of the user.
 * Used to determine the welcome message, Good morning / Good afternoon.
 * @return Current local hour.
 */
function get_local_time() {
  const hours = new Date().getHours();
  return hours;
}

const Index: Component = () => {
  const { userId, setUserId, username, setUsername } = useUser();
  const [filter, setFilter] = createSignal(0);

  const welcome_text = () => get_local_time() < 12 ? 'Good morning' : 'Good afternoon';
  const is_user = () => username() != "" ? true : false;
  const username_display = () => is_user() ? username() : "Guest";
  const level_display = () => is_user() && "Level 0";

  onMount(async () => {
    if (userId() !== -1) return;
    const user_data = await get_user_data_from_session(CACHE_DURATION, CACHE_KEY);
    setUserId(user_data.user_id);
    setUsername(user_data.username);
  });

  return (
    <div class={styles.game_container}>
      <div class={styles.play_container}>

      </div>
      <div class={styles.user_container}>
        <span class={styles.header_text}>{welcome_text()},</span>
        <span class={`${styles.s_sub_header_text} ${styles.user_display_container}`}>
          {username_display()}
          <hr class={styles.divider} />
          {level_display()}
        </span>
        <div class={styles.category_container}>
          <div class={styles.category_header}>
            <span class={`${styles.sub_header_text} ${styles.text_outline}`}>Categories</span>
            <FilterMenu filter={filter} setFilter={setFilter}/>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Index;