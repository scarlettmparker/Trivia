import MenuComponent from "../MenuComponent/menucomponent";
import Navbar from "../Navbar";
import WelcomeMessage from "../WelcomeMessage";

import styles from "~/routes/admin/admin.module.css";

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

export default AdminPlaceHolder;