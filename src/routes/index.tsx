import { createSignal, onMount, type Component } from 'solid-js';
import { CACHE_DURATION, CACHE_KEY, ENV } from '~/const';
import { useUser } from '~/usercontext';
import { get_user_data_from_session } from './utils/userutils';
import { Title } from '@solidjs/meta';
import FilterMenu from '~/components/FilterMenu';
import Category from '~/components/Category';
import styles from './index.module.css';

/**
 * Helper function to get the user's local time.
 * Used to determine the welcome message, Good morning / Good afternoon.
 * @return Current local hour.
 */
function get_local_time() {
  const hours = new Date().getHours();
  return hours;
}

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

const Index: Component = () => {
  const { userId, setUserId, username, setUsername } = useUser();
  const [filter, setFilter] = createSignal(0);

  const welcome_text = () => get_local_time() < 12 ? 'Good morning' : 'Good afternoon';
  const is_user = () => username() != "" ? true : false;
  const username_display = () => is_user() ? username() : "Guest";
  const level_display = () => is_user() && "Level 0";

  // Fetch user data from session on mount
  onMount(async () => {
    if (userId() !== -1) return;
    const user_data = await get_user_data_from_session(CACHE_DURATION, CACHE_KEY);
    setUserId(user_data.user_id);
    setUsername(user_data.username);
  });

  return (
    <>
      <Title>Trivia | Home</Title>
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
      </div>
    </>
  );
};

export default Index;