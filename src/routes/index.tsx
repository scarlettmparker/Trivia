import { createEffect, createSignal, onMount, type Component } from 'solid-js';
import { CACHE_DURATION, CACHE_KEY, ENV } from '~/const';
import { useUser } from '~/usercontext';
import { get_user_data_from_session } from './utils/userutils';
import { Title } from '@solidjs/meta';
import FilterMenu from '~/components/FilterMenu';
import Category from '~/components/Category';
import styles from './index.module.css';
import WelcomeMessage from '~/components/WelcomeMessage';
import Navbar from '~/components/Navbar';

const Index: Component = () => {
  const { userId, setUserId, setUsername } = useUser();
  const [filter, setFilter] = createSignal(0);
  const [loading, setLoading] = createSignal(true);

  // fetch user data from session on mount
  onMount(async () => {
    if (userId() !== -1) {
      setLoading(false);
      return;
    }

    const user_data = await get_user_data_from_session(CACHE_DURATION, CACHE_KEY);
    
    setUserId(user_data.user_id);
    setUsername(user_data.username);
    setLoading(false);
  });

  return (
    <>
      <Title>Trivia | Home</Title>
      {loading() ? (
        <Navbar placeholder={true} />) : (userId() !== -1 ? <Navbar /> : <Navbar placeholder={true} />
      )}
      <div class={styles.game_container}>
        <div class={styles.user_container}>
          <WelcomeMessage />
          <div class={styles.category_container}>
            <div class={styles.category_header}>
              <span class={`${styles.sub_header_text} ${styles.text_outline}`}>Categories</span>
              <FilterMenu filter={filter} setFilter={setFilter} />
            </div>
            <div class={styles.category_list}>
              <Category category_name={"Geography"} />
              <Category category_name={"History"} />
              <Category category_name={"Entertainment"} />
              <Category category_name={"Music"} />
            </div>
          </div>
        </div>
        <div class={styles.play_container}>

        </div>
      </div>
    </>
  );
};

export default Index;