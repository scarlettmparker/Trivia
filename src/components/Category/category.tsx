import ButtonWithAlt from '../ButtonWithAlt';
import styles from './category.module.css';

interface CategoryProps {
  category_name: string
}

const Category = ({ category_name }: CategoryProps) => {
  const FAVOURITE_SIZE = 16;
  const EXPAND_WIDTH = 20;
  const EXPAND_HEIGHT = 16;

  return (
    <div class={styles.category_container}>
      <div class={styles.category_header}>
        <span class={styles.body_text}>{category_name}</span>
        <ButtonWithAlt src={() => "/assets/favourite_icon.png"} alt_text={() => "Favourite"}
          class={styles.favourite_icon} button_class={styles.favourite_button}
          width={FAVOURITE_SIZE} height={FAVOURITE_SIZE} draggable={false} />
      </div>
      <span class={styles.sub_body_text}>0/300 complete</span>
      <ButtonWithAlt src={() => "/assets/expand_button.svg"} alt_text={() => "More Details"}
        class={styles.expand_icon} button_class={styles.expand_button}
        width={EXPAND_WIDTH} height={EXPAND_HEIGHT} draggable={false} />
    </div>
  )
};

export default Category;