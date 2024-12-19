import ListComponentProps from './listcomponentprops';
import styles from './listcomponent.module.css';

const ListComponent = ({ name, id, class: class_, onclick: onclick_, currentmenu }: ListComponentProps) => {
  return (
    <div class={currentmenu() == id ? styles.list_component_selected : styles.list_component} onclick={onclick_}>
      {name}
    </div>
  );
}

export default ListComponent;