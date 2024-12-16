import { useUser } from "~/usercontext";
import styles from './welcomemessage.module.css';

/**
 * Helper function to get the user's local time.
 * Used to determine the welcome message, Good morning / Good afternoon.
 * @return Current local hour.
 */
function get_local_time() {
  const hours = new Date().getHours();
  return hours;
}

interface WelcomeMessageProps {
  class?: string;
  admin?: boolean;
}

const WelcomeMessage = ({ class: class_, admin }: WelcomeMessageProps) => {
  const { username } = useUser();

  const welcome_text = () => get_local_time() < 12 ? 'Good morning' : 'Good afternoon';
  const is_user = () => username() != "" ? true : false;
  const username_display = () => is_user() ? username() : "Guest";
  const level_display = () => admin ? "Admin" : "Level 0";

  return (
    <div class={class_} style={{ display: 'flex', "flex-direction": 'column' }}>
      <span class={styles.header_text}>{welcome_text()},</span>
      <span class={`${styles.s_sub_header_text} ${styles.user_display_container}`}>
        {username_display()}
        <hr class={styles.divider} />
        {level_display()}
      </span>
    </div>
  )
}

export default WelcomeMessage;