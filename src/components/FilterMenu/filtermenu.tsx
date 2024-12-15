import { Accessor } from 'solid-js';
import ButtonWithAlt from '~/components/ButtonWithAlt';
import styles from './filtermenu.module.css';

interface FilterMenuProps {
  filter: Accessor<number>;
  setFilter: (filter: number) => void;
}

function get_next_elem(curr_index: number, direction: number, map: { [key: number]: string }) {
  const map_size = Object.keys(map).length;
  const next_elem = (curr_index + direction + map_size) % map_size;
  return next_elem;
}

const filter_map: { [key: number]: string } = {
  0: "A-Z",
  1: "Recent",
  2: "Z-A"
};

const BUTTON_SIZE = 18;

/**
 * Filter menu component. This allows the user to filter between sorts of the categories.
 * @param filter Accessor for the current filter value.
 * @param setFilter Setter for the current filter value.
 * @returns JSX Element - filter menu with two alt-text buttons, one on either side
 */
const FilterMenu = ({filter, setFilter}: FilterMenuProps) => {
  return (
    <span class={`${styles.body_text} ${styles.filter_container}`}>
      <ButtonWithAlt src={() => "/assets/filter_icon.svg"} alt_text={() => "Prev"}
        class={`${styles.filter_icon_left} ${styles.filter_icon}`} button_class={styles.filter_button} width={BUTTON_SIZE} height={BUTTON_SIZE}
        draggable={false} onclick={() => setFilter(get_next_elem(filter(), -1, filter_map))} />
      <span class={styles.filter_text}>{filter_map[filter()]}</span>
      <ButtonWithAlt src={() => "/assets/filter_icon_right.svg"} alt_text={() => "Next"}
        class={`${styles.filter_icon_right} ${styles.filter_icon}`} button_class={styles.filter_button}
        width={BUTTON_SIZE} height={BUTTON_SIZE} draggable={false} onclick={() => setFilter(get_next_elem(filter(), 1, filter_map))} />
    </span>
  )
}

export default FilterMenu;