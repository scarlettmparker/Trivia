import { useUser } from '~/usercontext';
import { logout } from '~/routes/utils/userutils';
import { useNavigate } from '@solidjs/router';
import styles from './navbar.module.css';

const Navbar = ({ placeholder }: { placeholder?: boolean }) => {
  const { userId, setUserId, setUsername } = useUser();
  const navigate = useNavigate();

  const redirect = async () => {
    if (await logout(userId(), setUserId, setUsername)) {
      navigate('/');
    }
  }

  return (
    <div class={styles.nav_bar}>
      {placeholder! ? (
        <span class={styles.nav_bar_item} onclick={() => navigate('/login')}>Login</span>
      ) : (
        <span class={styles.nav_bar_item} onclick={async () => await redirect()}>Logout</span>
      )}
    </div>
  )
}

export default Navbar;