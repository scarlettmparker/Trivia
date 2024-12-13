import { Title } from "@solidjs/meta";
import { createSignal } from "solid-js";
import styles from "./login.module.css";

function login(username: string, password: string) {

}

const Login = () => {
  const [username, setUsername] = createSignal("");
  const [password, setPassword] = createSignal("");
  const [showpassword, setShowpassword] = createSignal(true);

  const EYE_WIDTH = 32;

  return (
    <div>
      <Title>Trivia | Login</Title>
      <div class={styles.login_container}>
        <span class={styles.welcome_text}>Welcome back!</span>
        <span class={styles.body_text}>Enter your details below to login to your account.</span>
        <div class={styles.login_form}>
          <input class={`${styles.input_text} ${styles.input_form}`} placeholder="Username" 
            value={username()} oninput={(e) => setUsername(e.target.value)}/>
          <div class={styles.password_form}>
            { password() && <img class={styles.show_password} src="./eye.png" width={EYE_WIDTH} height={EYE_WIDTH} draggable={false}
              onclick={() => setShowpassword(!showpassword())} /> }
            <input class={`${styles.input_text} ${styles.input_form}`} placeholder="Password" type={showpassword() ? "password" : "text"}
              value={password()} oninput={(e) => setPassword(e.target.value)}/>
          </div>
        </div>
        <span class={`${styles.body_text} ${styles.forgot_text}`}>
          Forgot<a class={styles.highlight_text}> username</a> / <a class={styles.highlight_text}>password</a>
        </span>
        <button class={`${styles.form_button} ${styles.button_text}`} onclick={() => login(username(), password())}>Login</button>
        <span class={`${styles.body_text} ${styles.no_account_text}`}>
          Don't have an account? <a class={styles.highlight_text}>Register</a>
        </span>
      </div>
    </div>
  )
};

export default Login;