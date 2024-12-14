import { Title } from "@solidjs/meta";
import { createSignal, onMount } from "solid-js";
import styles from "./login.module.css";
import ButtonWithAlt from '~/components/ButtonWithAlt';

const ENV = {
  VITE_SERVER_HOST: import.meta.env.VITE_SERVER_HOST,
  VITE_SERVER_PORT: import.meta.env.VITE_SERVER_PORT
};

async function login(username: string, password: string, setError: (error: string) => void) {
  const response = await fetch(`http://${ENV.VITE_SERVER_HOST}:${ENV.VITE_SERVER_PORT}/api/user`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    },
    body: JSON.stringify({
      username: username, password: password
    })
  });

  const data = await response.json();
  if (data.status == 'ok') {
    window.location.href = '/';
  } else {
    setError(data.message);
  }
}

const Login = () => {
  const [username, setUsername] = createSignal("");
  const [password, setPassword] = createSignal("");
  const [showpassword, setShowpassword] = createSignal(true);
  const [error, setError] = createSignal("");

  const EYE_SIZE = 32;
  const EYE_SOURCE = () => showpassword() ? './assets/eye.png' : './assets/eye_closed.png';
  const ALT_TEXT = () => showpassword() ? 'Show password' : 'Hide password';
  const ALT_ONCLICK = () => setShowpassword(!showpassword());

  const handleKeyDown = (e: KeyboardEvent) => {
    if (e.key === 'Enter') {
      login(username(), password(), setError);
    }
  };

  onMount(() => {
    document.addEventListener('keydown', handleKeyDown);
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
    }
  });

  return (
    <div>
      <Title>Trivia | Login</Title>
      <div class={styles.login_container}>
        <span class={styles.welcome_text}>Welcome back!</span>
        <span class={styles.body_text}>Enter your details below to login to your account.</span>
        <div class={styles.login_form}>
          <input class={`${styles.input_text} ${styles.input_form}`} placeholder="Username"
            value={username()} oninput={(e) => setUsername(e.target.value)} />
          <div class={styles.password_form}>
            {password() &&
              <ButtonWithAlt src={EYE_SOURCE} alt_text={ALT_TEXT} class={styles.show_password}
                button_class={styles.password_button} width={EYE_SIZE} height={EYE_SIZE} draggable={false} onclick={ALT_ONCLICK} />
            }
            <input class={`${styles.input_text} ${styles.input_form}`} placeholder="Password" type={showpassword() ? "password" : "text"}
              value={password()} oninput={(e) => setPassword(e.target.value)} />
          </div>
        </div>
        <div class={styles.forgot_form}>
          <span class={`${styles.body_text} ${styles.error_text}`}>{error()}</span>
          <span class={`${styles.body_text} ${styles.forgot_text}`}>
            Forgot<a class={styles.highlight_text}> username</a> / <a class={styles.highlight_text}>password</a>
          </span>
        </div>
        <button class={`${styles.form_button} ${styles.button_text}`} onclick={() => login(username(), password(), setError)}>Login</button>
        <span class={`${styles.body_text} ${styles.no_account_text}`}>
          Don't have an account? <a class={styles.highlight_text}>Register</a>
        </span>
      </div>
    </div>
  )
};

export default Login;