import ButtonWithAlt from '~/components/ButtonWithAlt';
import styles from './filtermenu.module.css';
import FilterMenuProps from './filtermenuprops';

/**
 * Get the next element in the array of filters.
 * @param curr_index Current index in the array.
 * @param direction Direction to move in the array.
 * @param map_size Size of the array.
 * @returns Next index in the array.
 */
function get_next_elem(curr_index: number, direction: number, map_size: number) {
  return (curr_index + direction + map_size) % map_size;
}

/**
 * Filter menu component. This allows the user to filter between sorts of the categories.
 * @param filter Accessor for the current filter value.
 * @param setFilter Setter for the current filter value.
 * @returns JSX Element - filter menu with two alt-text buttons, one on either side
 */
const FilterMenu = ({ filter, setFilter }: FilterMenuProps) => {
  const filters = [
    "A-Z",
    "Recent",
    "Z-A"
  ];

  const BUTTON_SIZE = 18;
  const map_size = filters.length;

  const handle_prev = () => {
    setFilter(get_next_elem(filter(), -1, map_size));
  }

  const handle_next = () => {
    setFilter(get_next_elem(filter(), 1, map_size));
  }

  return (
    <span class={`${styles.body_text} ${styles.filter_container}`}>
      <ButtonWithAlt src={() => "/assets/filter_icon.svg"} alt_text={() => "Prev"}
        class={`${styles.filter_icon_left} ${styles.filter_icon}`} button_class={styles.filter_button}
        width={BUTTON_SIZE} height={BUTTON_SIZE} draggable={false} onclick={handle_prev} />
      <span class={styles.filter_text}>{filters[filter()]}</span>
      <ButtonWithAlt src={() => "/assets/filter_icon_right.svg"} alt_text={() => "Next"}
        class={`${styles.filter_icon_right} ${styles.filter_icon}`} button_class={styles.filter_button}
        width={BUTTON_SIZE} height={BUTTON_SIZE} draggable={false} onclick={handle_next} />
    </span>
  )
}

export default FilterMenu;